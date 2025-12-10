#include "orchestrator.h"
#include "schedule.h"
#include <iostream>
#include <signal.h>

using namespace orchestrator;

// Global orchestrator pointer for signal handling
Orchestrator* g_orchestrator = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_orchestrator) {
        g_orchestrator->stop();
    }
    exit(0);
}

int main(int argc, char** argv) {
    std::cout << "=== gRPC Orchestrator ===" << std::endl;
    std::cout << "Starting orchestrator service..." << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create orchestrator
    std::string listen_address = "0.0.0.0:50050";
    if (argc > 1) {
        listen_address = argv[1];
    }
    
    Orchestrator orchestrator(listen_address);
    g_orchestrator = &orchestrator;
    
    // Load schedule
    TaskSchedule schedule;
    
    if (argc > 2) {
        // Load from file
        std::string schedule_file = argv[2];
        std::cout << "[Main] Loading schedule from: " << schedule_file << std::endl;
        schedule = ScheduleParser::parse_yaml(schedule_file);
    } else {
        // Use test schedule
        std::cout << "[Main] Using test schedule" << std::endl;
        schedule = ScheduleParser::create_test_schedule();
    }
    
    orchestrator.load_schedule(schedule);
    
    // Start orchestrator
    orchestrator.start();
    
    std::cout << "[Main] Orchestrator started, waiting for tasks to complete..." << std::endl;
    std::cout << "[Main] Press Ctrl+C to stop" << std::endl;
    
    // Wait for all tasks to complete
    orchestrator.wait_for_completion();
    
    // Print execution summary
    std::cout << "\n=== Execution Summary ===" << std::endl;
    auto history = orchestrator.get_execution_history();
    
    int success_count = 0;
    int failure_count = 0;
    
    for (const auto& exec : history) {
        std::cout << "Task: " << exec.task_id << std::endl;
        std::cout << "  Scheduled: " << exec.scheduled_time_us << " us" << std::endl;
        std::cout << "  Started: " << exec.actual_start_time_us << " us" << std::endl;
        std::cout << "  Ended: " << exec.end_time_us << " us" << std::endl;
        std::cout << "  Duration: " << (exec.end_time_us - exec.actual_start_time_us) << " us" << std::endl;
        std::cout << "  Result: " << exec.result << std::endl;
        
        if (exec.result == TASK_RESULT_SUCCESS) {
            success_count++;
        } else {
            failure_count++;
            if (!exec.error_message.empty()) {
                std::cout << "  Error: " << exec.error_message << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "Total tasks: " << history.size() << std::endl;
    std::cout << "Successful: " << success_count << std::endl;
    std::cout << "Failed: " << failure_count << std::endl;
    
    // Stop orchestrator
    orchestrator.stop();
    
    std::cout << "[Main] Orchestrator stopped" << std::endl;
    
    return 0;
}
