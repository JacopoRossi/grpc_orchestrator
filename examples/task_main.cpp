#include "task_wrapper.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace orchestrator;

// Global task wrapper pointer for signal handling
TaskWrapper* g_task_wrapper = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

// Example task execution function
TaskResult example_task_function(const std::map<std::string, std::string>& params) {
    std::cout << "[Task Function] Starting execution with parameters:" << std::endl;
    
    for (const auto& param : params) {
        std::cout << "  " << param.first << " = " << param.second << std::endl;
    }
    
    // Simulate some work
    int duration_ms = 500;  // Default duration
    
    auto it = params.find("duration_ms");
    if (it != params.end()) {
        try {
            duration_ms = std::stoi(it->second);
        } catch (...) {
            std::cerr << "[Task Function] Invalid duration_ms parameter" << std::endl;
        }
    }
    
    std::cout << "[Task Function] Simulating work for " << duration_ms << " ms..." << std::endl;
    
    // Simulate work in chunks to allow for interruption
    int chunks = duration_ms / 100;
    for (int i = 0; i < chunks; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[Task Function] Progress: " << ((i + 1) * 100 / chunks) << "%" << std::endl;
    }
    
    std::cout << "[Task Function] Work completed successfully" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <task_id> <listen_address> <orchestrator_address>" << std::endl;
        std::cerr << "Example: " << argv[0] 
                  << " task_1 localhost:50051 localhost:50050" << std::endl;
        return 1;
    }
    
    std::string task_id = argv[1];
    std::string listen_address = argv[2];
    std::string orchestrator_address = argv[3];
    
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
