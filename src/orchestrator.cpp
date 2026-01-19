#include "orchestrator.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <pthread.h>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace orchestrator {

// Helper functions
namespace {
    int64_t get_timestamp_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    void log_with_timestamp(const std::string& msg) {
        std::cout << "[" << std::setw(13) << get_timestamp_ms() << " ms] " << msg << std::endl;
    }
}

// ============================================================================
// OrchestratorServiceImpl Implementation
// ============================================================================

OrchestratorServiceImpl::OrchestratorServiceImpl(Orchestrator* orchestrator)
    : orchestrator_(orchestrator) {}

grpc::Status OrchestratorServiceImpl::NotifyTaskEnd(
    grpc::ServerContext* context,
    const TaskEndNotification* request,
    TaskEndResponse* response) {
    
    log_with_timestamp("← Task " + request->task_id() + 
                      " completed (result: " + std::to_string(request->result()) +
                      ", duration: " + std::to_string(request->execution_duration_us() / 1000.0) + " ms)");
    
    orchestrator_->on_task_end(*request);
    
    response->set_acknowledged(true);
    response->set_message("Task end notification received");
    
    return grpc::Status::OK;
}

grpc::Status OrchestratorServiceImpl::HealthCheck(
    grpc::ServerContext* context,
    const HealthCheckRequest* request,
    HealthCheckResponse* response) {
    
    response->set_healthy(orchestrator_->is_running());
    response->set_status(orchestrator_->is_running() ? "running" : "stopped");
    response->set_timestamp_us(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    return grpc::Status::OK;
}

// ============================================================================
// Orchestrator Implementation
// ============================================================================

Orchestrator::Orchestrator(const std::string& listen_address)
    : listen_address_(listen_address)
    , next_task_index_(0)
    , start_time_us_(0)
    , running_(false)
    , pending_tasks_(0)
    , last_task_end_time_us_(0) {
    
    service_ = std::make_unique<OrchestratorServiceImpl>(this);
}

Orchestrator::~Orchestrator() {
    stop();
}

void Orchestrator::load_schedule(const TaskSchedule& schedule) {
    std::lock_guard<std::mutex> lock(mutex_);
    schedule_ = schedule;
    schedule_.sort_by_time();
    next_task_index_ = 0;
    
    std::cout << "[Orchestrator] Loaded schedule with " 
              << schedule_.tasks.size() << " tasks" << std::endl;
}

void Orchestrator::set_rt_config(const RTConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    rt_config_ = config;
    
    std::cout << "[Orchestrator] Real-time configuration set:" << std::endl;
    std::cout << "  Policy: " << RTUtils::policy_to_string(config.policy) << std::endl;
    std::cout << "  Priority: " << config.priority << std::endl;
    std::cout << "  CPU Affinity: " << (config.cpu_affinity >= 0 ? std::to_string(config.cpu_affinity) : "none") << std::endl;
}

void Orchestrator::start() {
    if (running_.exchange(true)) {
        std::cout << "[Orchestrator] Already running" << std::endl;
        return;
    }
    
    std::cout << "[Orchestrator] Starting orchestrator on " 
              << listen_address_ << std::endl;
    
    // Start gRPC server in separate thread
    server_thread_ = std::thread([this]() {
        // Apply real-time configuration to server thread if requested
        if (rt_config_.policy != RT_POLICY_NONE) {
            RTConfig server_config = rt_config_;
            // Server thread typically has lower priority than scheduler
            if (server_config.priority > 1) {
                server_config.priority -= 1;
            }
            RTUtils::apply_rt_config(server_config);
        }
        
        grpc::ServerBuilder builder;
        builder.AddListeningPort(listen_address_, grpc::InsecureServerCredentials());
        builder.RegisterService(service_.get());
        
        server_ = builder.BuildAndStart();
        std::cout << "[Orchestrator] gRPC server listening on " 
                  << listen_address_ << std::endl;
        
        server_->Wait();
    });
    
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Start scheduler thread
    start_time_us_ = get_current_time_us();
    scheduler_thread_ = std::thread(&Orchestrator::scheduler_loop, this);
    
    std::cout << "[Orchestrator] Scheduler started" << std::endl;
}

void Orchestrator::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    std::cout << "[Orchestrator] Stopping orchestrator..." << std::endl;
    
    // Stop scheduler thread
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    // Stop gRPC server
    if (server_) {
        server_->Shutdown();
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    std::cout << "[Orchestrator] Orchestrator stopped" << std::endl;
}

void Orchestrator::wait_for_completion() {
    std::unique_lock<std::mutex> lock(mutex_);
    completion_cv_.wait(lock, [this]() {
        return pending_tasks_ == 0 && next_task_index_ >= schedule_.tasks.size();
    });
    
    std::cout << "[Orchestrator] All tasks completed" << std::endl;
}

std::vector<TaskExecution> Orchestrator::get_execution_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return completed_tasks_;
}

void Orchestrator::on_task_end(const TaskEndNotification& notification) {
    // Measure end time IMMEDIATELY when notification arrives
    int64_t task_end_time = get_current_time_us() - start_time_us_;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_tasks_.find(notification.task_id());
    if (it == active_tasks_.end()) {
        std::cerr << "[Orchestrator] Warning: received end notification for unknown task: "
                  << notification.task_id() << std::endl;
        return;
    }
    
    // Update task execution state
    TaskExecution& exec = it->second;
    exec.end_time_us = task_end_time;  // Measured by orchestrator
    exec.state = TASK_STATE_COMPLETED;
    exec.result = notification.result();
    exec.error_message = notification.error_message();
    exec.output_data_json = notification.output_data_json();
    
    // Update last task end time for context switch measurement
    last_task_end_time_us_ = task_end_time;
    
    // Store output for dependent tasks
    task_outputs_[notification.task_id()] = exec.output_data_json;
    if (!exec.output_data_json.empty() && exec.output_data_json != "{}") {
        std::cout << "[Orchestrator] Task " << notification.task_id() 
                  << " output: " << exec.output_data_json << std::endl;
    }
    
    // Move to completed tasks and mark as done
    completed_tasks_.push_back(exec);
    active_tasks_.erase(it);
    task_completed_[notification.task_id()] = true;
    
    // Notify waiting tasks and check for completion
    --pending_tasks_;
    task_end_cv_.notify_all();
    
    if (pending_tasks_ == 0 && next_task_index_ >= schedule_.tasks.size()) {
        completion_cv_.notify_all();
    }
}

void Orchestrator::scheduler_loop() {
    std::cout << "[Orchestrator] Scheduler loop started (HYBRID MODE)" << std::endl;
    std::cout << "[Orchestrator] Supporting both sequential and timed execution" << std::endl;
    
    // Apply real-time configuration to scheduler thread
    if (rt_config_.policy != RT_POLICY_NONE) {
        RTUtils::apply_rt_config(rt_config_);
    }
    
    // PHASE 1: Launch all TIMED tasks immediately (they will wait internally for their scheduled time)
    std::cout << "\n[Orchestrator] === PHASE 1: Launching TIMED tasks ===\n" << std::endl;
    for (const ScheduledTask& task : schedule_.tasks) {
        if (task.execution_mode == TASK_MODE_TIMED) {
            log_with_timestamp("→ Launching TIMED task: " + task.task_id + 
                             " (scheduled at " + std::to_string(task.scheduled_time_us / 1000) + " ms)");
            
            // Launch task on separate thread
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_tasks_++;
                next_task_index_++;
            }
            
            // Capture task by value and start_time_us for timing
            std::thread([this, task]() {
                // Wait until scheduled time
                int64_t current_time = get_current_time_us() - start_time_us_;
                int64_t wait_time_us = task.scheduled_time_us - current_time;
                
                if (wait_time_us > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(wait_time_us));
                }
                execute_task(task);
            }).detach();
        }
    }
    
    // PHASE 2: Process SEQUENTIAL tasks in order
    std::cout << "\n[Orchestrator] === PHASE 2: Processing SEQUENTIAL tasks ===\n" << std::endl;
    for (size_t i = 0; i < schedule_.tasks.size() && running_; i++) {
        const ScheduledTask& task = schedule_.tasks[i];
        
        if (task.execution_mode == TASK_MODE_SEQUENTIAL) {
            
            // Check if we need to wait for a dependency
            if (!task.wait_for_task_id.empty()) {
                log_with_timestamp("⏸ Waiting for " + task.wait_for_task_id + " to complete...");
                
                std::unique_lock<std::mutex> lock(mutex_);
                task_end_cv_.wait(lock, [this, &task]() {
                    return task_completed_[task.wait_for_task_id] || !running_;
                });
                
                if (!running_) {
                    std::cout << "[Orchestrator] Scheduler interrupted" << std::endl;
                    break;
                }
                
                log_with_timestamp("✓ Dependency satisfied, " + task.wait_for_task_id + " completed");
            }
            
            // Log task launch
            std::string msg = "→ Launching SEQUENTIAL task: " + task.task_id;
            if (!task.wait_for_task_id.empty()) {
                msg += " (after " + task.wait_for_task_id + ")";
            }
            log_with_timestamp(msg);
            
            // Launch task on separate thread
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_tasks_++;
                next_task_index_++;
            }
            
            std::thread([this, task]() {
                execute_task(task);
            }).detach();
            
            // Wait for task to be registered in active_tasks first
            {
                std::unique_lock<std::mutex> lock(mutex_);
                task_end_cv_.wait_for(lock, std::chrono::milliseconds(100), [this, &task]() {
                    return active_tasks_.find(task.task_id) != active_tasks_.end() || !running_;
                });
            }
            
            // Wait for completion
            {
                std::unique_lock<std::mutex> lock(mutex_);
                task_end_cv_.wait(lock, [this, &task]() {
                    return active_tasks_.find(task.task_id) == active_tasks_.end() || !running_;
                });
                
                if (!running_) {
                    std::cout << "[Orchestrator] Scheduler interrupted" << std::endl;
                    break;
                }
            }
        }
    }
    
    // Wait for all remaining tasks to complete
    std::unique_lock<std::mutex> lock(mutex_);
    task_end_cv_.wait(lock, [this]() {
        return pending_tasks_ == 0 || !running_;
    });
    
    std::cout << "\n[Orchestrator] ========================================" << std::endl;
    std::cout << "[Orchestrator] All tasks completed successfully!" << std::endl;
    std::cout << "[Orchestrator] ========================================\n" << std::endl;
    
    // Print context switch statistics
    std::cout << "\n[Orchestrator] === Context Switch Statistics ===" << std::endl;
    int64_t total_context_switch_time = 0;
    int64_t min_context_switch = INT64_MAX;
    int64_t max_context_switch = 0;
    int context_switch_count = 0;
    
    for (const auto& task : completed_tasks_) {
        if (task.context_switch_time_us > 0) {
            total_context_switch_time += task.context_switch_time_us;
            min_context_switch = std::min(min_context_switch, task.context_switch_time_us);
            max_context_switch = std::max(max_context_switch, task.context_switch_time_us);
            context_switch_count++;
            
            std::cout << "[Orchestrator] Task " << task.task_id 
                      << ": " << task.context_switch_time_us << " µs ("
                      << (task.context_switch_time_us / 1000.0) << " ms)" << std::endl;
        }
    }
    
    if (context_switch_count > 0) {
        double avg_context_switch = static_cast<double>(total_context_switch_time) / context_switch_count;
        std::cout << "\n[Orchestrator] Context Switch Summary:" << std::endl;
        std::cout << "  - Count: " << context_switch_count << std::endl;
        std::cout << "  - Average: " << avg_context_switch << " µs (" 
                  << (avg_context_switch / 1000.0) << " ms)" << std::endl;
        std::cout << "  - Min: " << min_context_switch << " µs (" 
                  << (min_context_switch / 1000.0) << " ms)" << std::endl;
        std::cout << "  - Max: " << max_context_switch << " µs (" 
                  << (max_context_switch / 1000.0) << " ms)" << std::endl;
        std::cout << "  - Total: " << total_context_switch_time << " µs (" 
                  << (total_context_switch_time / 1000.0) << " ms)" << std::endl;
    }
    std::cout << "[Orchestrator] ===================================\n" << std::endl;
    
    completion_cv_.notify_all();
}

void Orchestrator::execute_task(const ScheduledTask& task) {
    // Measure start time BEFORE any communication
    int64_t task_start_time = get_current_time_us() - start_time_us_;
    
    // Register task BEFORE sending start command to avoid race condition
    {
        std::lock_guard<std::mutex> lock(mutex_);
        TaskExecution exec;
        exec.task_id = task.task_id;
        exec.scheduled_time_us = task.scheduled_time_us;
        exec.actual_start_time_us = task_start_time;  // Measured by orchestrator
        exec.estimated_duration_us = task.estimated_duration_us;
        exec.state = TASK_STATE_STARTING;
        exec.result = TASK_RESULT_UNKNOWN;
        
        // Calculate context switch time (time between previous task end and this task start)
        if (last_task_end_time_us_ > 0) {
            exec.context_switch_time_us = task_start_time - last_task_end_time_us_;
            
            // Log context switch time
            std::cout << "[Orchestrator] ⏱️  Context Switch Time: " 
                      << exec.context_switch_time_us << " µs ("
                      << (exec.context_switch_time_us / 1000.0) << " ms)" << std::endl;
        } else {
            exec.context_switch_time_us = 0;  // First task, no context switch
        }
        
        active_tasks_[task.task_id] = exec;
        
        // Notify that task has been registered
        task_end_cv_.notify_one();
    }
    
    // Create gRPC stub for task
    auto channel = grpc::CreateChannel(task.task_address, grpc::InsecureChannelCredentials());
    auto stub = TaskService::NewStub(channel);
    
    // Prepare start request
    StartTaskRequest request;
    request.set_task_id(task.task_id);
    request.set_scheduled_time_us(task.scheduled_time_us);
    request.set_deadline_us(task.deadline_us);
    request.set_rt_policy(task.rt_policy);
    request.set_rt_priority(task.rt_priority);
    request.set_cpu_affinity(task.cpu_affinity);
    
    // Set task parameters JSON
    std::string params_json = task.parameters_json;
    
    // Add output from dependent task if available
    if (!task.wait_for_task_id.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto output_it = task_outputs_.find(task.wait_for_task_id);
        if (output_it != task_outputs_.end() && !output_it->second.empty()) {
            std::cout << "[Orchestrator] Passing output from " << task.wait_for_task_id 
                      << " to " << task.task_id << std::endl;
            
            try {
                // Parse both JSONs
                json params = params_json.empty() ? json::object() : json::parse(params_json);
                json dep_output = json::parse(output_it->second);
                
                // Merge dependency output into parameters under "dep_output" key
                params["dep_output"] = dep_output;
                
                params_json = params.dump();
            } catch (const json::exception& e) {
                std::cerr << "[Orchestrator] JSON merge error: " << e.what() << std::endl;
            }
        }
    }
    
    request.set_parameters_json(params_json);
    
    StartTaskResponse response;
    grpc::ClientContext context;
    
    // Set timeout for gRPC call
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);
    
    // Send start command
    grpc::Status status = stub->StartTask(&context, request, &response);
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_tasks_.find(task.task_id);
    
    if (status.ok() && response.success()) {
        // Task started successfully - update execution state
        // Note: Time is measured by orchestrator, not by task
        if (it != active_tasks_.end()) {
            it->second.state = TASK_STATE_RUNNING;
        }
    } else {
        // Mark task as failed
        std::cerr << "[Orchestrator] Failed to start task " << task.task_id 
                  << ": " << status.error_message() << std::endl;
        
        int64_t now = get_current_time_us() - start_time_us_;
        TaskExecution exec{task.task_id, task.scheduled_time_us, now, now, 
                          task.estimated_duration_us, 0, TASK_STATE_FAILED, 
                          TASK_RESULT_FAILURE, status.error_message()};
        
        completed_tasks_.push_back(exec);
        
        if (--pending_tasks_ == 0 && next_task_index_ >= schedule_.tasks.size()) {
            completion_cv_.notify_all();
        }
    }
}

int64_t Orchestrator::get_current_time_us() const {
    // Use system_clock for global synchronization across containers
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace orchestrator
