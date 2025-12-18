#pragma once

#include "schedule.h"
#include "orchestrator.grpc.pb.h"
#include "rt_utils.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <unordered_map>

namespace orchestrator {

// Task execution tracking
struct TaskExecution {
    std::string task_id;
    int64_t scheduled_time_us;
    int64_t actual_start_time_us;
    int64_t end_time_us;
    TaskState state;
    TaskResult result;
    std::string error_message;
    std::map<std::string, std::string> output_data;
};

// Orchestrator service implementation (receives task end notifications)
class OrchestratorServiceImpl final : public OrchestratorService::Service {
public:
    OrchestratorServiceImpl(class Orchestrator* orchestrator);
    
    grpc::Status NotifyTaskEnd(
        grpc::ServerContext* context,
        const TaskEndNotification* request,
        TaskEndResponse* response) override;
    
    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const HealthCheckRequest* request,
        HealthCheckResponse* response) override;

private:
    class Orchestrator* orchestrator_;
};

// Main Orchestrator class
class Orchestrator {
public:
    Orchestrator(const std::string& listen_address = "0.0.0.0:50050");
    ~Orchestrator();
    
    // Load and set the task schedule
    void load_schedule(const TaskSchedule& schedule);
    
    // Set real-time configuration for orchestrator threads
    void set_rt_config(const RTConfig& config);
    
    // Start the orchestrator (begins scheduling tasks)
    void start();
    
    // Stop the orchestrator
    void stop();
    
    // Wait for all tasks to complete
    void wait_for_completion();
    
    // Get execution statistics
    std::vector<TaskExecution> get_execution_history() const;
    
    // Called by service when task ends
    void on_task_end(const TaskEndNotification& notification);
    
    // Check if orchestrator is running
    bool is_running() const { return running_; }
    
    // Get relative time since start (in microseconds)
    int64_t get_relative_time_us() const { return get_current_time_us() - start_time_us_; }

private:
    // Scheduler thread function
    void scheduler_loop();
    
    // Execute a scheduled task (send start command via gRPC)
    void execute_task(const ScheduledTask& task);
    
    // Get current time in microseconds
    int64_t get_current_time_us() const;
    
    // gRPC server for receiving notifications
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<OrchestratorServiceImpl> service_;
    std::string listen_address_;
    
    // Schedule data
    TaskSchedule schedule_;
    size_t next_task_index_;
    int64_t start_time_us_;
    
    // Threading
    std::atomic<bool> running_;
    std::thread scheduler_thread_;
    std::thread server_thread_;
    
    // Task tracking
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TaskExecution> active_tasks_;
    std::vector<TaskExecution> completed_tasks_;
    std::unordered_map<std::string, bool> task_completed_;  // Track completed tasks for dependencies
    std::unordered_map<std::string, std::map<std::string, std::string>> task_outputs_;  // Store task outputs
    
    // Synchronization
    std::condition_variable completion_cv_;
    std::condition_variable task_end_cv_;  // For sequential execution
    std::atomic<int> pending_tasks_;
    
    // Real-time configuration
    RTConfig rt_config_;
};

} // namespace orchestrator
