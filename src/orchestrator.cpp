#include "orchestrator.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace orchestrator {

// ============================================================================
// OrchestratorServiceImpl Implementation
// ============================================================================

OrchestratorServiceImpl::OrchestratorServiceImpl(Orchestrator* orchestrator)
    : orchestrator_(orchestrator) {}

grpc::Status OrchestratorServiceImpl::NotifyTaskEnd(
    grpc::ServerContext* context,
    const TaskEndNotification* request,
    TaskEndResponse* response) {
    
    std::cout << "[Orchestrator] Received task end notification: " 
              << request->task_id() << std::endl;
    
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
    , pending_tasks_(0) {
    
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

void Orchestrator::start() {
    if (running_.exchange(true)) {
        std::cout << "[Orchestrator] Already running" << std::endl;
        return;
    }
    
    std::cout << "[Orchestrator] Starting orchestrator on " 
              << listen_address_ << std::endl;
    
    // Start gRPC server in separate thread
    server_thread_ = std::thread([this]() {
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
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_tasks_.find(notification.task_id());
    if (it == active_tasks_.end()) {
        std::cerr << "[Orchestrator] Warning: received end notification for unknown task: "
                  << notification.task_id() << std::endl;
        return;
    }
    
    TaskExecution& exec = it->second;
    exec.end_time_us = notification.end_time_us();
    exec.state = TASK_STATE_COMPLETED;
    exec.result = notification.result();
    exec.error_message = notification.error_message();
    
    std::cout << "[Orchestrator] Task " << notification.task_id() 
              << " completed with result: " << notification.result()
              << " (duration: " << notification.execution_duration_us() << " us)"
              << std::endl;
    
    // Move to completed tasks
    completed_tasks_.push_back(exec);
    active_tasks_.erase(it);
    
    // Decrement pending tasks counter
    if (--pending_tasks_ == 0 && next_task_index_ >= schedule_.tasks.size()) {
        completion_cv_.notify_all();
    }
}

void Orchestrator::scheduler_loop() {
    std::cout << "[Orchestrator] Scheduler loop started" << std::endl;
    
    while (running_) {
        int64_t current_time_us = get_current_time_us() - start_time_us_;
        
        // Check if there are tasks to schedule
        std::lock_guard<std::mutex> lock(mutex_);
        
        while (next_task_index_ < schedule_.tasks.size()) {
            const ScheduledTask& task = schedule_.tasks[next_task_index_];
            
            if (task.scheduled_time_us <= current_time_us) {
                std::cout << "[Orchestrator] Scheduling task: " << task.task_id 
                          << " at time " << current_time_us << " us" << std::endl;
                
                // Execute task in separate thread to avoid blocking scheduler
                std::thread([this, task]() {
                    execute_task(task);
                }).detach();
                
                next_task_index_++;
                pending_tasks_++;
            } else {
                // Next task is in the future
                break;
            }
        }
        
        // Check if all tasks are done
        if (next_task_index_ >= schedule_.tasks.size() && pending_tasks_ == 0) {
            std::cout << "[Orchestrator] All tasks scheduled and completed" << std::endl;
            completion_cv_.notify_all();
            break;
        }
        
        // Sleep for a short time (1ms) to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "[Orchestrator] Scheduler loop ended" << std::endl;
}

void Orchestrator::execute_task(const ScheduledTask& task) {
    // Register task BEFORE sending start command to avoid race condition
    {
        std::lock_guard<std::mutex> lock(mutex_);
        TaskExecution exec;
        exec.task_id = task.task_id;
        exec.scheduled_time_us = task.scheduled_time_us;
        exec.actual_start_time_us = get_current_time_us();
        exec.state = TASK_STATE_STARTING;
        exec.result = TASK_RESULT_UNKNOWN;
        
        active_tasks_[task.task_id] = exec;
    }
    
    // Create gRPC stub for task
    auto channel = grpc::CreateChannel(task.task_address, grpc::InsecureChannelCredentials());
    auto stub = TaskService::NewStub(channel);
    
    // Prepare start request
    StartTaskRequest request;
    request.set_task_id(task.task_id);
    request.set_scheduled_time_us(task.scheduled_time_us);
    request.set_deadline_us(task.deadline_us);
    request.set_priority(task.priority);
    
    for (const auto& param : task.parameters) {
        (*request.mutable_parameters())[param.first] = param.second;
    }
    
    StartTaskResponse response;
    grpc::ClientContext context;
    
    // Set timeout for gRPC call
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);
    
    // Send start command
    grpc::Status status = stub->StartTask(&context, request, &response);
    
    if (status.ok() && response.success()) {
        std::cout << "[Orchestrator] Task " << task.task_id 
                  << " started successfully" << std::endl;
        
        // Update task execution state
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_tasks_.find(task.task_id);
        if (it != active_tasks_.end()) {
            it->second.actual_start_time_us = response.actual_start_time_us();
            it->second.state = TASK_STATE_RUNNING;
        }
    } else {
        std::cerr << "[Orchestrator] Failed to start task " << task.task_id 
                  << ": " << status.error_message() << std::endl;
        
        // Mark task as failed
        std::lock_guard<std::mutex> lock(mutex_);
        TaskExecution exec;
        exec.task_id = task.task_id;
        exec.scheduled_time_us = task.scheduled_time_us;
        exec.actual_start_time_us = get_current_time_us();
        exec.end_time_us = exec.actual_start_time_us;
        exec.state = TASK_STATE_FAILED;
        exec.result = TASK_RESULT_FAILURE;
        exec.error_message = status.error_message();
        
        completed_tasks_.push_back(exec);
        
        if (--pending_tasks_ == 0 && next_task_index_ >= schedule_.tasks.size()) {
            completion_cv_.notify_all();
        }
    }
}

int64_t Orchestrator::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace orchestrator
