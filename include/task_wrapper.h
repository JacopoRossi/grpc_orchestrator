#pragma once

#include "orchestrator.grpc.pb.h"
#include "rt_utils.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace orchestrator {

// Task execution callback type - returns result and output data
using TaskExecutionCallback = std::function<TaskResult(const std::map<std::string, std::string>&, 
                                                        std::map<std::string, std::string>&)>;

// Task service implementation (receives start/stop commands)
class TaskServiceImpl final : public TaskService::Service {
public:
    TaskServiceImpl(class TaskWrapper* wrapper);
    
    grpc::Status StartTask(
        grpc::ServerContext* context,
        const StartTaskRequest* request,
        StartTaskResponse* response) override;
    
    grpc::Status StopTask(
        grpc::ServerContext* context,
        const StopTaskRequest* request,
        StopTaskResponse* response) override;
    
    grpc::Status GetTaskStatus(
        grpc::ServerContext* context,
        const TaskStatusRequest* request,
        TaskStatusResponse* response) override;

private:
    class TaskWrapper* wrapper_;
};

// Task Wrapper class
class TaskWrapper {
public:
    TaskWrapper(
        const std::string& task_id,
        const std::string& listen_address,
        const std::string& orchestrator_address,
        TaskExecutionCallback execution_callback);
    
    ~TaskWrapper();
    
    // Set real-time configuration for task execution thread
    void set_rt_config(const RTConfig& config);
    
    // Start the task wrapper (listen for commands)
    void start();
    
    // Stop the task wrapper
    void stop();
    
    // Execute the task (called by service when .start is received)
    void execute_task(const StartTaskRequest& request);
    
    // Get current task state
    TaskState get_state() const { return state_; }
    
    // Get task ID
    std::string get_task_id() const { return task_id_; }
    
    // Get execution statistics
    int64_t get_start_time_us() const { return start_time_us_; }
    int64_t get_elapsed_time_us() const;
    
    // Get relative time since wrapper creation (for logging)
    int64_t get_relative_time_ms() const;

private:
    // Task execution thread function
    void task_execution_thread(StartTaskRequest request);
    
    // Send task end notification to orchestrator
    void notify_orchestrator_end(TaskResult result, const std::string& error_msg = "", 
                                  const std::map<std::string, std::string>& output_data = {});
    
    // Get current time in microseconds
    int64_t get_current_time_us() const;
    
    // Task identification
    std::string task_id_;
    
    // gRPC server for receiving commands
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<TaskServiceImpl> service_;
    std::string listen_address_;
    
    // gRPC client for notifying orchestrator
    std::unique_ptr<OrchestratorService::Stub> orchestrator_stub_;
    std::string orchestrator_address_;
    
    // Task execution
    TaskExecutionCallback execution_callback_;
    std::thread execution_thread_;
    
    // State management
    std::atomic<TaskState> state_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_requested_;
    
    // Timing
    int64_t start_time_us_;
    int64_t end_time_us_;
    int64_t creation_time_us_;  // Time when wrapper was created (for logging)
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Real-time configuration
    RTConfig rt_config_;
};

} // namespace orchestrator
