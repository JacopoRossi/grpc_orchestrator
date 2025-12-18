#include "task_wrapper.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <pthread.h>

namespace orchestrator {

// ============================================================================
// TaskServiceImpl Implementation
// ============================================================================

TaskServiceImpl::TaskServiceImpl(TaskWrapper* wrapper)
    : wrapper_(wrapper) {}

grpc::Status TaskServiceImpl::StartTask(
    grpc::ServerContext* context,
    const StartTaskRequest* request,
    StartTaskResponse* response) {
    
    std::cout << "[" << std::setw(13) << wrapper_->get_relative_time_ms() << " ms] "
              << "[Task " << wrapper_->get_task_id() 
              << "] Received start command" << std::endl;
    
    if (wrapper_->get_state() != TASK_STATE_IDLE) {
        response->set_success(false);
        response->set_message("Task is not in IDLE state");
        return grpc::Status::OK;
    }
    
    // Execute task
    wrapper_->execute_task(*request);
    
    response->set_success(true);
    response->set_message("Task started");
    response->set_actual_start_time_us(wrapper_->get_start_time_us());
    response->set_task_id(wrapper_->get_task_id());
    
    return grpc::Status::OK;
}

grpc::Status TaskServiceImpl::StopTask(
    grpc::ServerContext* context,
    const StopTaskRequest* request,
    StopTaskResponse* response) {
    
    std::cout << "[" << std::setw(13) << wrapper_->get_relative_time_ms() << " ms] "
              << "[Task " << wrapper_->get_task_id() 
              << "] Received stop command" << std::endl;
    
    // Request graceful stop
    wrapper_->stop();
    
    response->set_success(true);
    response->set_message("Stop requested");
    response->set_stop_time_us(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    return grpc::Status::OK;
}

grpc::Status TaskServiceImpl::GetTaskStatus(
    grpc::ServerContext* context,
    const TaskStatusRequest* request,
    TaskStatusResponse* response) {
    
    response->set_task_id(wrapper_->get_task_id());
    response->set_state(wrapper_->get_state());
    response->set_start_time_us(wrapper_->get_start_time_us());
    response->set_elapsed_time_us(wrapper_->get_elapsed_time_us());
    response->set_cpu_usage_percent(0.0);  // TODO: implement CPU monitoring
    response->set_memory_usage_bytes(0);   // TODO: implement memory monitoring
    
    return grpc::Status::OK;
}

// ============================================================================
// TaskWrapper Implementation
// ============================================================================

TaskWrapper::TaskWrapper(
    const std::string& task_id,
    const std::string& listen_address,
    const std::string& orchestrator_address,
    TaskExecutionCallback execution_callback)
    : task_id_(task_id)
    , listen_address_(listen_address)
    , orchestrator_address_(orchestrator_address)
    , execution_callback_(execution_callback)
    , state_(TASK_STATE_IDLE)
    , running_(false)
    , stop_requested_(false)
    , start_time_us_(0)
    , end_time_us_(0)
    , creation_time_us_(get_current_time_us()) {
    
    service_ = std::make_unique<TaskServiceImpl>(this);
    
    // Create stub for orchestrator
    auto channel = grpc::CreateChannel(
        orchestrator_address_, 
        grpc::InsecureChannelCredentials());
    orchestrator_stub_ = OrchestratorService::NewStub(channel);
    
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Task wrapper created" << std::endl;
}

TaskWrapper::~TaskWrapper() {
    stop();
}

void TaskWrapper::set_rt_config(const RTConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    rt_config_ = config;
    
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Real-time configuration set:" << std::endl;
    std::cout << "  Policy: " << RTUtils::policy_to_string(config.policy) << std::endl;
    std::cout << "  Priority: " << config.priority << std::endl;
    std::cout << "  CPU Affinity: " << (config.cpu_affinity >= 0 ? std::to_string(config.cpu_affinity) : "none") << std::endl;
}

void TaskWrapper::start() {
    if (running_.exchange(true)) {
        std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Already running" << std::endl;
        return;
    }
    
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Starting task wrapper on " 
              << listen_address_ << std::endl;
    
    // Start gRPC server
    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    
    server_ = builder.BuildAndStart();
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] gRPC server listening on " 
              << listen_address_ << std::endl;
    
    state_ = TASK_STATE_IDLE;
}

void TaskWrapper::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Stopping task wrapper..." << std::endl;
    
    stop_requested_ = true;
    
    // Wait for execution thread to finish
    if (execution_thread_.joinable()) {
        execution_thread_.join();
    }
    
    // Stop gRPC server
    if (server_) {
        server_->Shutdown();
    }
    
    state_ = TASK_STATE_STOPPED;
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Task wrapper stopped" << std::endl;
}

void TaskWrapper::execute_task(const StartTaskRequest& request) {
    // Start execution in separate thread
    if (execution_thread_.joinable()) {
        execution_thread_.join();
    }
    
    execution_thread_ = std::thread(&TaskWrapper::task_execution_thread, this, request);
}

void TaskWrapper::task_execution_thread(StartTaskRequest request) {
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Starting task execution" << std::endl;
    
    // Apply real-time configuration from request (if specified)
    if (request.rt_policy() != "none" && !request.rt_policy().empty()) {
        RTConfig rt_config;
        rt_config.policy = RTUtils::string_to_policy(request.rt_policy());
        rt_config.priority = request.rt_priority();
        rt_config.cpu_affinity = request.cpu_affinity();
        
        std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Applying RT config: policy=" 
                  << request.rt_policy() << ", priority=" << request.rt_priority()
                  << ", cpu_affinity=" << request.cpu_affinity() << std::endl;
        
        if (!RTUtils::apply_rt_config(rt_config)) {
            std::cerr << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                      << "[Task " << task_id_ << "] Warning: Failed to apply RT configuration" << std::endl;
        }
    } else if (rt_config_.policy != RT_POLICY_NONE) {
        // Fallback to wrapper-level RT config if no request-level config
        RTUtils::apply_rt_config(rt_config_);
    }
    
    state_ = TASK_STATE_STARTING;
    start_time_us_ = get_current_time_us();
    
    // Convert parameters to map
    std::map<std::string, std::string> params;
    for (const auto& param : request.parameters()) {
        params[param.first] = param.second;
    }
    
    // Add task_id to parameters so the callback can identify which task it is
    params["task_id"] = task_id_;
    
    state_ = TASK_STATE_RUNNING;
    
    // Execute the actual task
    TaskResult result = TASK_RESULT_UNKNOWN;
    std::string error_message;
    std::map<std::string, std::string> output_data;
    
    try {
        result = execution_callback_(params, output_data);
        
        if (result == TASK_RESULT_UNKNOWN) {
            result = TASK_RESULT_SUCCESS;
        }
        
        std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Task execution completed successfully" 
                  << std::endl;
    } catch (const std::exception& e) {
        result = TASK_RESULT_FAILURE;
        error_message = std::string("Exception: ") + e.what();
        std::cerr << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Task execution failed: " 
                  << error_message << std::endl;
    } catch (...) {
        result = TASK_RESULT_FAILURE;
        error_message = "Unknown exception";
        std::cerr << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Task execution failed with unknown exception" 
                  << std::endl;
    }
    
    end_time_us_ = get_current_time_us();
    
    // Check if stop was requested
    if (stop_requested_) {
        result = TASK_RESULT_CANCELLED;
        error_message = "Task cancelled by stop request";
    }
    
    state_ = TASK_STATE_COMPLETED;
    
    // Notify orchestrator
    notify_orchestrator_end(result, error_message, output_data);
    
    // Return to idle state
    state_ = TASK_STATE_IDLE;
}

void TaskWrapper::notify_orchestrator_end(TaskResult result, const std::string& error_msg, 
                                          const std::map<std::string, std::string>& output_data) {
    std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
              << "[Task " << task_id_ << "] Notifying orchestrator of task end" 
              << std::endl;
    
    TaskEndNotification notification;
    notification.set_task_id(task_id_);
    notification.set_result(result);
    notification.set_start_time_us(start_time_us_);
    notification.set_end_time_us(end_time_us_);
    notification.set_execution_duration_us(end_time_us_ - start_time_us_);
    notification.set_error_message(error_msg);
    
    // Add output data
    for (const auto& output : output_data) {
        (*notification.mutable_output_data())[output.first] = output.second;
    }
    
    TaskEndResponse response;
    grpc::ClientContext context;
    
    // Set timeout for gRPC call
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
    context.set_deadline(deadline);
    
    grpc::Status status = orchestrator_stub_->NotifyTaskEnd(&context, notification, &response);
    
    if (status.ok() && response.acknowledged()) {
        std::cout << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Orchestrator acknowledged task end" 
                  << std::endl;
    } else {
        std::cerr << "[" << std::setw(13) << get_relative_time_ms() << " ms] "
                  << "[Task " << task_id_ << "] Failed to notify orchestrator: " 
                  << status.error_message() << std::endl;
    }
}

int64_t TaskWrapper::get_elapsed_time_us() const {
    if (start_time_us_ == 0) {
        return 0;
    }
    
    if (state_ == TASK_STATE_RUNNING || state_ == TASK_STATE_STARTING) {
        return get_current_time_us() - start_time_us_;
    } else {
        return end_time_us_ - start_time_us_;
    }
}

int64_t TaskWrapper::get_current_time_us() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

int64_t TaskWrapper::get_relative_time_ms() const {
    // Return absolute timestamp in milliseconds for synchronization
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace orchestrator
