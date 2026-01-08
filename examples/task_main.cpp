#include "task_wrapper.h"
#include "rt_utils.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <signal.h>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace orchestrator;

// Global task wrapper pointer for signal handling
TaskWrapper* g_task_wrapper = nullptr;

// Helper function to get absolute timestamp in milliseconds (for synchronization with orchestrator)
int64_t get_absolute_time_ms() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now;
}

void signal_handler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

// Example task execution function
TaskResult example_task_function(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task Function] Starting execution with parameters: " << params_json << std::endl;
    
    // Parse input JSON
    json params;
    try {
        params = json::parse(params_json);
    } catch (const json::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task Function] JSON parse error: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    // Get task_id to determine which logic to execute
    std::string task_id = params.value("task_id", "");
    
    // Initialize output JSON
    json output = json::object();
    
    // Execute task-specific logic
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "\n========================================" << std::endl;
    
    if (task_id == "task_1") {
        // Task 1: input = number, output = number * 5
        if (!params.contains("input")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1] ERROR: Missing 'input' parameter" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        try {
            int input_value = params["input"].is_string() ? std::stoi(params["input"].get<std::string>()) : params["input"].get<int>();
            
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1] Input: " << input_value << std::endl;
            
            // Simula carico computazionale con loop
            long long prev = 0, curr = 1, sum = 0;
            for (int i = 0; i < 10000000; i++) {
                sum = prev + curr;
                prev = curr;
                curr = sum;
                if (i % 100000 == 0) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    //std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                              //<< "[Task 1] Loop iteration " << i << ", sum = " << sum << std::endl;
                }
            }
            
            int output_value = input_value * 5;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1] Output: " << input_value << " * 5 = " << output_value << std::endl;
            
            output["result"] = output_value;
            
        } catch (const std::exception& e) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1] ERROR: Invalid input value: " << e.what() << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
    } else if (task_id == "task_2") {
        // Task 2: input = number, output = number + 1
        if (!params.contains("input")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2] ERROR: Missing 'input' parameter" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        try {
            int input_value = params["input"].is_string() ? std::stoi(params["input"].get<std::string>()) : params["input"].get<int>();   

            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2] Input: " << input_value << std::endl;
            
            // Simula carico computazionale con loop
            long long prev = 0, curr = 1, sum = 0;
            for (int i = 0; i < 10000000; i++) {
                sum = prev + curr;
                prev = curr;
                curr = sum;
                if (i % 100000 == 0) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    //std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                              //<< "[Task 2] Loop iteration " << i << ", sum = " << sum << std::endl;
                }
            }
            
            int output_value = input_value + 1;
            
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2] Output: " << input_value << " + 1 = " << output_value << std::endl;
            
            output["result"] = output_value;
            
        } catch (const std::exception& e) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2] ERROR: Invalid input value: " << e.what() << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
    } else if (task_id == "task_3") {
        // Task 3: input1 = output from task_2 (dep_output.result), input2 = another number, output = input1 * input2
        if (!params.contains("dep_output") || !params.contains("input2")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] ERROR: Missing 'dep_output' or 'input2' parameter" << std::endl;
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] Available parameters: " << params.dump() << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        try {
            json dep_output = params["dep_output"];
            if (!dep_output.contains("result")) {
                std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                          << "[Task 3] ERROR: Missing 'result' in dep_output" << std::endl;
                return TASK_RESULT_FAILURE;
            }
            
            int input1_value = dep_output["result"].is_string() ? std::stoi(dep_output["result"].get<std::string>()) : dep_output["result"].get<int>();
            int input2_value = params["input2"].is_string() ? std::stoi(params["input2"].get<std::string>()) : params["input2"].get<int>();
            
            // Sleep di 5 secondi
            
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            
            int output_value = input1_value * input2_value;
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] Input1 (from task_2): " << input1_value << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] Input2: " << input2_value << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] Output: " << input1_value << " * " << input2_value << " = " << output_value << std::endl;
            
            output["result"] = output_value;
            
        } catch (const std::exception& e) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3] ERROR: Invalid input values: " << e.what() << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
    } else {
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "Unknown task: " << task_id << std::endl;
    }
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "========================================\n" << std::endl;
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task Function] Task completed successfully" << std::endl;
    
    // Convert output to JSON string
    output_json = output.dump();
    
    return TASK_RESULT_SUCCESS;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --name <id>             Task ID" << std::endl;
    std::cout << "  --address <addr>        Listen address" << std::endl;
    std::cout << "  --orchestrator <addr>   Orchestrator address" << std::endl;
    std::cout << "\nReal-Time Options:" << std::endl;
    std::cout << "  --policy <policy>       RT scheduling policy: none, fifo, rr (default: none)" << std::endl;
    std::cout << "  --priority <n>          RT priority: 1-99 (default: 50)" << std::endl;
    std::cout << "  --cpu-affinity <n>      Bind to CPU core (default: -1, no affinity)" << std::endl;
    std::cout << "  --lock-memory           Lock memory pages (prevents page faults)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
    std::cout << "\nBackward Compatible Usage:" << std::endl;
    std::cout << "  " << program_name << " <task_id> <listen_address> <orchestrator_address>" << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    std::string task_id;
    std::string listen_address;
    std::string orchestrator_address;
    RTConfig rt_config;
    
    // Backward compatibility: positional arguments
    if (argc >= 4 && argv[1][0] != '-') {
        task_id = argv[1];
        listen_address = argv[2];
        orchestrator_address = argv[3];
    } else {
        // New style: named arguments
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
    }
    
    // Validate required arguments
    if (task_id.empty() || listen_address.empty() || orchestrator_address.empty()) {
        std::cerr << "Error: Missing required arguments" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    std::cout << "=== gRPC Task Wrapper ===" << std::endl;
    std::cout << "Task ID: " << task_id << std::endl;
    std::cout << "Listen Address: " << listen_address << std::endl;
    std::cout << "Orchestrator Address: " << orchestrator_address << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create task wrapper
    TaskWrapper task_wrapper(
        task_id,
        listen_address,
        orchestrator_address,
        example_task_function
    );
    
    g_task_wrapper = &task_wrapper;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Main] Configuring real-time scheduling" << std::endl;
        task_wrapper.set_rt_config(rt_config);
    } else {
        std::cout << "[Main] Running in non-real-time mode" << std::endl;
    }
    
    // Start task wrapper (listen for commands)
    task_wrapper.start();
    
    std::cout << "[Main] Task wrapper started, waiting for commands..." << std::endl;
    std::cout << "[Main] Press Ctrl+C to stop" << std::endl;
    
    // Keep running until stopped
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Check if we should exit (task wrapper stopped)
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;
        }
    }
    
    std::cout << "[Main] Task wrapper stopped" << std::endl;
    
    return 0;
}
