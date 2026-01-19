#include "schedule.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace orchestrator {

// Helper function to convert YAML node to JSON value while preserving types
static json yaml_to_json(const YAML::Node& node) {
    if (node.IsNull()) return nullptr;
    
    if (node.IsScalar()) {
        try { return node.as<int>(); } catch (...) {}
        try { return node.as<double>(); } catch (...) {}
        try { return node.as<bool>(); } catch (...) {}
        return node.as<std::string>();
    }
    
    if (node.IsSequence()) {
        json arr = json::array();
        for (const auto& item : node) arr.push_back(yaml_to_json(item));
        return arr;
    }
    
    if (node.IsMap()) {
        json obj = json::object();
        for (const auto& pair : node) {
            obj[pair.first.as<std::string>()] = yaml_to_json(pair.second);
        }
        return obj;
    }
    
    return node.as<std::string>();
}

template<typename T>
T get_or_default(const YAML::Node& node, const std::string& key, T default_val) {
    return node[key] ? node[key].as<T>() : default_val;
}

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
            YAML::Node defaults = sched["defaults"];
            int64_t default_deadline_us = get_or_default(defaults, "deadline_us", 1000000L);
            std::string default_rt_policy = get_or_default<std::string>(defaults, "rt_policy", "none");
            int default_rt_priority = get_or_default(defaults, "rt_priority", 50);
            int default_cpu_affinity = get_or_default(defaults, "cpu_affinity", -1);
            
            // Check if running in Docker
            const char* docker_env = std::getenv("DOCKER_CONTAINER");
            bool use_docker_hostnames = (docker_env != nullptr);
            
            // Parse tasks
            if (sched["tasks"]) {
                YAML::Node tasks = sched["tasks"];
                
                for (const auto& task_node : tasks) {
                    ScheduledTask task;
                    
                    // Required fields
                    task.task_id = task_node["id"].as<std::string>();
                    task.task_address = task_node["address"].as<std::string>();
                    
                    // Execution mode
                    std::string mode = task_node["mode"].as<std::string>();
                    task.execution_mode = (mode == "sequential") ? TASK_MODE_SEQUENTIAL : TASK_MODE_TIMED;
                    task.scheduled_time_us = (mode == "timed") ? task_node["scheduled_time_us"].as<int64_t>() : 0;
                    
                    // Dependencies
                    task.wait_for_task_id = get_or_default<std::string>(task_node, "depends_on", "");
                    
                    // Optional fields with defaults
                    task.deadline_us = get_or_default(task_node, "deadline_us", default_deadline_us);
                    task.estimated_duration_us = get_or_default(task_node, "estimated_duration_us", 1000000L);
                    
                    // Real-time configuration
                    task.rt_policy = get_or_default(task_node, "rt_policy", default_rt_policy);
                    task.rt_priority = get_or_default(task_node, "rt_priority", default_rt_priority);
                    task.cpu_affinity = get_or_default(task_node, "cpu_affinity", default_cpu_affinity);
                    
                    // Parameters - convert to JSON
                    json params_obj = json::object();
                    if (task_node["parameters"]) {
                        YAML::Node params = task_node["parameters"];
                        for (YAML::const_iterator it = params.begin(); it != params.end(); ++it) {
                            params_obj[it->first.as<std::string>()] = yaml_to_json(it->second);
                        }
                    }
                    
                    // Add task_id to parameters
                    params_obj["task_id"] = task.task_id;
                    task.parameters_json = params_obj.dump();
                    
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
    
    bool use_docker = (std::getenv("DOCKER_CONTAINER") != nullptr);
    auto addr = [use_docker](const std::string& host, int port) {
        return use_docker ? host + ":" + std::to_string(port) : "localhost:" + std::to_string(port);
    };
    
    // Task 1: Execute immediately (sequential mode)
    ScheduledTask task1{"task_1", addr("task1", 50051), 0, 3000000, 
                        json{{"mode", "fast"}, {"iterations", "100"}, {"task_id", "task_1"}}.dump(),
                        TASK_MODE_SEQUENTIAL, "", 500000, "none", 50, -1};
    
    // Task 2: Execute at 8 seconds (timed mode)
    ScheduledTask task2{"task_2", addr("task2", 50052), 8000000, 1000000,
                        json{{"mode", "normal"}, {"data_size", "1024"}, {"task_id", "task_2"}}.dump(),
                        TASK_MODE_TIMED, "", 800000, "none", 50, -1};
    
    // Task 3: Execute after task_1 completes (sequential mode)
    ScheduledTask task3{"task_3", addr("task3", 50053), 0, 5000000,
                        json{{"mode", "slow"}, {"quality", "high"}, {"task_id", "task_3"}}.dump(),
                        TASK_MODE_SEQUENTIAL, "task_1", 1500000, "none", 50, -1};
    
    schedule.tasks = {task1, task2, task3};
    
    std::cout << "[ScheduleParser] Created test schedule with " << schedule.tasks.size() 
              << " tasks" << (use_docker ? " (Docker mode)" : " (Local mode)") << std::endl;
    
    return schedule;
}

} // namespace orchestrator
