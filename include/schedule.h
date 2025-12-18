#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>

namespace orchestrator {

// Execution mode for a task
enum TaskExecutionMode {
    TASK_MODE_SEQUENTIAL,    // Wait for previous task to complete
    TASK_MODE_TIMED          // Execute at specific scheduled time
};

// Represents a scheduled task execution
struct ScheduledTask {
    std::string task_id;
    std::string task_address;          // gRPC address (e.g., "localhost:50051")
    int64_t scheduled_time_us;         // Scheduled start time in microseconds
    int64_t deadline_us;               // Task deadline in microseconds
    int32_t priority;                  // Task priority
    std::map<std::string, std::string> parameters;  // Task parameters
    TaskExecutionMode execution_mode;  // Sequential or timed execution
    std::string wait_for_task_id;      // Task ID to wait for (if sequential)
    
    // Optional metadata
    int64_t estimated_duration_us;     // Estimated execution time
    int32_t max_retries;               // Maximum retry attempts
    bool critical;                     // Is this a critical task?
    
    // Real-time configuration
    std::string rt_policy;             // RT scheduling policy: "none", "fifo", "rr", "deadline"
    int32_t rt_priority;               // RT priority (1-99, 99 = highest)
    int32_t cpu_affinity;              // CPU core to bind to (-1 = no affinity)
};

// Represents the complete schedule
struct TaskSchedule {
    int64_t time_horizon_start_us;     // Schedule start time
    int64_t time_horizon_end_us;       // Schedule end time
    int64_t tick_duration_us;          // Duration of one tick
    std::vector<ScheduledTask> tasks;  // List of scheduled tasks
    
    // Sort tasks by scheduled time
    void sort_by_time() {
        std::sort(tasks.begin(), tasks.end(), 
            [](const ScheduledTask& a, const ScheduledTask& b) {
                return a.scheduled_time_us < b.scheduled_time_us;
            });
    }
};

// Helper class to parse schedule from YAML or other formats
class ScheduleParser {
public:
    // Parse schedule from YAML file
    static TaskSchedule parse_yaml(const std::string& yaml_path);
    
    // Parse schedule from JSON string
    static TaskSchedule parse_json(const std::string& json_str);
    
    // Create a simple test schedule
    static TaskSchedule create_test_schedule();
};

} // namespace orchestrator
