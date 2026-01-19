#include "task_wrapper.h"
#include "rt_utils.h"
#include "my_tasks.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <map>
#include <functional>

using namespace orchestrator;

// Global task wrapper pointer for signal handling
TaskWrapper* g_task_wrapper = nullptr;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nGeneric Task Runner - Executes tasks defined in my_tasks.h" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --name <id>             Task ID (e.g., 'task_1', 'task_2', 'task_3')" << std::endl;
    std::cout << "  --address <addr>        Listen address (e.g., '0.0.0.0:50051')" << std::endl;
    std::cout << "  --orchestrator <addr>   Orchestrator address (e.g., 'orchestrator:50050')" << std::endl;
    std::cout << "\nReal-Time Options:" << std::endl;
    std::cout << "  --policy <policy>       RT scheduling policy: none, fifo, rr (default: none)" << std::endl;
    std::cout << "  --priority <n>          RT priority: 1-99 (default: 50)" << std::endl;
    std::cout << "  --cpu-affinity <n>      Bind to CPU core (default: -1, no affinity)" << std::endl;
    std::cout << "  --lock-memory           Lock memory pages (prevents page faults)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
    std::cout << "\nAvailable Tasks:" << std::endl;
    std::cout << "  task_1                  Data Analyzer" << std::endl;
    std::cout << "  task_2                  Image Processor" << std::endl;
    std::cout << "  task_3                  Report Generator" << std::endl;
    std::cout << "  custom                  Custom Task (modify my_tasks.h)" << std::endl;
}

int main(int argc, char** argv) {
    std::string task_id;
    std::string listen_address;
    std::string orchestrator_address;
    RTConfig rt_config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--name" && i + 1 < argc) {
            task_id = argv[++i];
        } else if (arg == "--address" && i + 1 < argc) {
            listen_address = argv[++i];
        } else if (arg == "--orchestrator" && i + 1 < argc) {
            orchestrator_address = argv[++i];
        } else if (arg == "--policy" && i + 1 < argc) {
            rt_config.policy = RTUtils::string_to_policy(argv[++i]);
        } else if (arg == "--priority" && i + 1 < argc) {
            rt_config.priority = std::stoi(argv[++i]);
        } else if (arg == "--cpu-affinity" && i + 1 < argc) {
            rt_config.cpu_affinity = std::stoi(argv[++i]);
        } else if (arg == "--lock-memory") {
            rt_config.lock_memory = true;
            rt_config.prefault_stack = true;
        }
    }
    
    // Validate required arguments
    if (task_id.empty() || listen_address.empty() || orchestrator_address.empty()) {
        std::cerr << "Error: Missing required arguments" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    // Map task IDs to their execution functions
    std::map<std::string, std::function<TaskResult(const std::string&, std::string&)>> task_registry;
    task_registry["task_1"] = task1_data_analyzer;
    task_registry["task_2"] = task2_image_processor;
    task_registry["task_3"] = task3_report_generator;
    task_registry["custom"] = my_custom_task;
    
    // Find the task function
    auto it = task_registry.find(task_id);
    if (it == task_registry.end()) {
        std::cerr << "Error: Unknown task ID '" << task_id << "'" << std::endl;
        std::cerr << "Available tasks: task_1, task_2, task_3, custom" << std::endl;
        return 1;
    }
    
    std::cout << "=== Generic Task Runner ===" << std::endl;
    std::cout << "Task ID: " << task_id << std::endl;
    std::cout << "Listen Address: " << listen_address << std::endl;
    std::cout << "Orchestrator Address: " << orchestrator_address << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create task wrapper with the selected function
    TaskWrapper task_wrapper(
        task_id,
        listen_address,
        orchestrator_address,
        it->second  // Use the function from registry
    );
    
    g_task_wrapper = &task_wrapper;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Task Runner] Configuring real-time scheduling" << std::endl;
        task_wrapper.set_rt_config(rt_config);
    } else {
        std::cout << "[Task Runner] Running in non-real-time mode" << std::endl;
    }
    
    // Start task wrapper
    task_wrapper.start();
    
    std::cout << "[Task Runner] Ready and waiting for commands..." << std::endl;
    std::cout << "[Task Runner] Press Ctrl+C to stop" << std::endl;
    
    // Keep running until stopped
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;
        }
    }
    
    std::cout << "[Task Runner] Stopped" << std::endl;
    
    return 0;
}
