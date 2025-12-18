#include "schedule.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace orchestrator {

TaskSchedule ScheduleParser::parse_yaml(const std::string& yaml_path) {
    std::cout << "[ScheduleParser] Parsing YAML file: " << yaml_path << std::endl;
    
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        TaskSchedule schedule;
        
        // Parse schedule metadata
        if (config["schedule"]) {
            YAML::Node sched = config["schedule"];
            
            if (sched["name"]) {
                std::cout << "[ScheduleParser] Schedule name: " << sched["name"].as<std::string>() << std::endl;
            }
            
            if (sched["description"]) {
                std::cout << "[ScheduleParser] Description: " << sched["description"].as<std::string>() << std::endl;
            }
            
            // Parse defaults
            int default_priority = 50;
            int default_max_retries = 3;
            bool default_critical = false;
            int64_t default_deadline_us = 1000000;
            std::string default_rt_policy = "none";
            int default_rt_priority = 50;
            int default_cpu_affinity = -1;
            
            if (sched["defaults"]) {
                YAML::Node defaults = sched["defaults"];
                if (defaults["priority"]) default_priority = defaults["priority"].as<int>();
                if (defaults["max_retries"]) default_max_retries = defaults["max_retries"].as<int>();
                if (defaults["critical"]) default_critical = defaults["critical"].as<bool>();
                if (defaults["deadline_us"]) default_deadline_us = defaults["deadline_us"].as<int64_t>();
                if (defaults["rt_policy"]) default_rt_policy = defaults["rt_policy"].as<std::string>();
                if (defaults["rt_priority"]) default_rt_priority = defaults["rt_priority"].as<int>();
                if (defaults["cpu_affinity"]) default_cpu_affinity = defaults["cpu_affinity"].as<int>();
            }
            
            // Check if running in Docker
            const char* docker_env = std::getenv("DOCKER_CONTAINER");
            bool use_docker_hostnames = (docker_env != nullptr);
            
            // Parse tasks
            if (sched["tasks"]) {
                YAML::Node tasks = sched["tasks"];
                
                for (size_t i = 0; i < tasks.size(); i++) {
                    YAML::Node task_node = tasks[i];
                    ScheduledTask task;
                    
                    // Required fields
                    task.task_id = task_node["id"].as<std::string>();
                    
                    // Address: use as-is from YAML (no conversion)
                    task.task_address = task_node["address"].as<std::string>();
                    
                    // Execution mode
                    std::string mode = task_node["mode"].as<std::string>();
                    if (mode == "sequential") {
                        task.execution_mode = TASK_MODE_SEQUENTIAL;
                        task.scheduled_time_us = 0;
                    } else if (mode == "timed") {
                        task.execution_mode = TASK_MODE_TIMED;
                        task.scheduled_time_us = task_node["scheduled_time_us"].as<int64_t>();
                    }
                    
                    // Dependencies
                    if (task_node["depends_on"]) {
                        task.wait_for_task_id = task_node["depends_on"].as<std::string>();
                    } else {
                        task.wait_for_task_id = "";
                    }
                    
                    // Optional fields with defaults
                    task.priority = task_node["priority"] ? task_node["priority"].as<int>() : default_priority;
                    task.max_retries = task_node["max_retries"] ? task_node["max_retries"].as<int>() : default_max_retries;
                    task.critical = task_node["critical"] ? task_node["critical"].as<bool>() : default_critical;
                    task.deadline_us = task_node["deadline_us"] ? task_node["deadline_us"].as<int64_t>() : default_deadline_us;
                    task.estimated_duration_us = task_node["estimated_duration_us"] ? task_node["estimated_duration_us"].as<int64_t>() : 1000000;
                    
                    // Real-time configuration
                    task.rt_policy = task_node["rt_policy"] ? task_node["rt_policy"].as<std::string>() : default_rt_policy;
                    task.rt_priority = task_node["rt_priority"] ? task_node["rt_priority"].as<int>() : default_rt_priority;
                    task.cpu_affinity = task_node["cpu_affinity"] ? task_node["cpu_affinity"].as<int>() : default_cpu_affinity;
                    
                    // Parameters
                    if (task_node["parameters"]) {
                        YAML::Node params = task_node["parameters"];
                        for (YAML::const_iterator it = params.begin(); it != params.end(); ++it) {
                            task.parameters[it->first.as<std::string>()] = it->second.as<std::string>();
                        }
                    }
                    
                    // Add task_id to parameters
                    task.parameters["task_id"] = task.task_id;
                    
                    schedule.tasks.push_back(task);
                    
                    std::cout << "[ScheduleParser] Loaded task: " << task.task_id 
                              << " (" << mode << ")" << std::endl;
                }
            }
        }
        
        // Set schedule time horizon
        schedule.time_horizon_start_us = 0;
        schedule.time_horizon_end_us = 3600000000;  // 1 hour default
        schedule.tick_duration_us = 1000;  // 1ms
        
        std::cout << "[ScheduleParser] Successfully loaded " << schedule.tasks.size() 
                  << " tasks from YAML" << std::endl;
        
        return schedule;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "[ScheduleParser] YAML parsing error: " << e.what() << std::endl;
        std::cerr << "[ScheduleParser] Falling back to test schedule" << std::endl;
        return create_test_schedule();
    }
}

TaskSchedule ScheduleParser::parse_json(const std::string& json_str) {
    std::cout << "[ScheduleParser] Parsing JSON string" << std::endl;
    
    // TODO: Implement JSON parsing
    // For now, return a test schedule
    std::cerr << "[ScheduleParser] Warning: JSON parsing not fully implemented, "
              << "using test schedule" << std::endl;
    
    return create_test_schedule();
}

TaskSchedule ScheduleParser::create_test_schedule() {
    TaskSchedule schedule;
    
    schedule.time_horizon_start_us = 0;
    schedule.time_horizon_end_us = 10000000;  // 10 seconds
    schedule.tick_duration_us = 1000;         // 1ms
    
    // Check if running in Docker (use hostname) or locally (use localhost)
    const char* docker_env = std::getenv("DOCKER_CONTAINER");
    bool use_docker_hostnames = (docker_env != nullptr);
    
    // Task 1: Execute immediately (sequential mode)
    ScheduledTask task1;
    task1.task_id = "task_1";
    task1.task_address = use_docker_hostnames ? "task1:50051" : "localhost:50051";
    task1.scheduled_time_us = 0;        // Start immediately
    task1.deadline_us = 3000000;        // 2 seconds
    task1.priority = 10;
    task1.parameters["mode"] = "fast";
    task1.parameters["iterations"] = "100";
    task1.parameters["task_id"] = "task_1";
    task1.estimated_duration_us = 500000;  // 500ms
    task1.max_retries = 3;
    task1.critical = true;
    task1.execution_mode = TASK_MODE_SEQUENTIAL;  // Sequential
    task1.wait_for_task_id = "";        // No dependency
    task1.rt_policy = "none";
    task1.rt_priority = 50;
    task1.cpu_affinity = -1;
    
    // Task 2: Execute at 2 seconds (timed mode)
    ScheduledTask task2;
    task2.task_id = "task_2";
    task2.task_address = use_docker_hostnames ? "task2:50052" : "localhost:50052";
    task2.scheduled_time_us = 8000000;  // 2 seconds from start
    task2.deadline_us = 1000000;        // 3 seconds
    task2.priority = 10;
    task2.parameters["mode"] = "normal";
    task2.parameters["data_size"] = "1024";
    task2.parameters["task_id"] = "task_2";
    task2.estimated_duration_us = 800000;  // 800ms
    task2.max_retries = 2;
    task2.critical = false;
    task2.execution_mode = TASK_MODE_TIMED;  // Timed execution
    task2.wait_for_task_id = "";        // No dependency
    task2.rt_policy = "none";
    task2.rt_priority = 50;
    task2.cpu_affinity = -1;
    
    // Task 3: Execute after task_1 completes (sequential mode)
    ScheduledTask task3;
    task3.task_id = "task_3";
    task3.task_address = use_docker_hostnames ? "task3:50053" : "localhost:50053";
    task3.scheduled_time_us = 0;        // Time doesn't matter in sequential mode
    task3.deadline_us = 5000000;        // 5 seconds
    task3.priority = 8;
    task3.parameters["mode"] = "slow";
    task3.parameters["quality"] = "high";
    task3.parameters["task_id"] = "task_3";
    task3.estimated_duration_us = 1500000;  // 1.5 seconds
    task3.max_retries = 1;
    task3.critical = true;
    task3.execution_mode = TASK_MODE_SEQUENTIAL;  // Sequential
    task3.wait_for_task_id = "task_1";  // Wait for task_1 to complete
    task3.rt_policy = "none";
    task3.rt_priority = 50;
    task3.cpu_affinity = -1;
    
    schedule.tasks.push_back(task1);
    schedule.tasks.push_back(task2);
    schedule.tasks.push_back(task3);
    
    std::cout << "[ScheduleParser] Created test schedule with " 
              << schedule.tasks.size() << " tasks" 
              << (use_docker_hostnames ? " (Docker mode)" : " (Local mode)")
              << std::endl;
    
    return schedule;
}

} // namespace orchestrator
