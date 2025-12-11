#include "orchestrator.h"
#include "schedule.h"
#include "rt_utils.h"
#include <iostream>
#include <signal.h>
#include <cstring>

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

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --address <addr>        Listen address (default: 0.0.0.0:50050)" << std::endl;
    std::cout << "  --schedule <file>       Schedule file path (YAML)" << std::endl;
    std::cout << "  --policy <policy>       RT scheduling policy: none, fifo, rr (default: none)" << std::endl;
    std::cout << "  --priority <n>          RT priority: 1-99 (default: 50)" << std::endl;
    std::cout << "  --cpu-affinity <n>      Bind to CPU core (default: -1, no affinity)" << std::endl;
    std::cout << "  --lock-memory           Lock memory pages (prevents page faults)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== gRPC Orchestrator ===" << std::endl;
    std::cout << "Starting orchestrator service..." << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments
    std::string listen_address = "0.0.0.0:50050";
    std::string schedule_file;
    RTConfig rt_config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--address" && i + 1 < argc) {
            listen_address = argv[++i];
        } else if (arg == "--schedule" && i + 1 < argc) {
            schedule_file = argv[++i];
        } else if (arg == "--policy" && i + 1 < argc) {
            rt_config.policy = RTUtils::string_to_policy(argv[++i]);
        } else if (arg == "--priority" && i + 1 < argc) {
            rt_config.priority = std::stoi(argv[++i]);
        } else if (arg == "--cpu-affinity" && i + 1 < argc) {
            rt_config.cpu_affinity = std::stoi(argv[++i]);
        } else if (arg == "--lock-memory") {
            rt_config.lock_memory = true;
            rt_config.prefault_stack = true;
        } else if (i == 1 && arg[0] != '-') {
            // Backward compatibility: first positional arg is address
            listen_address = arg;
        } else if (i == 2 && arg[0] != '-' && schedule_file.empty()) {
            // Backward compatibility: second positional arg is schedule file
            schedule_file = arg;
        }
    }
    
    Orchestrator orchestrator(listen_address);
    g_orchestrator = &orchestrator;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Main] Configuring real-time scheduling" << std::endl;
        orchestrator.set_rt_config(rt_config);
    } else {
        std::cout << "[Main] Running in non-real-time mode" << std::endl;
    }
    
    // Load schedule
    TaskSchedule schedule;
    
    if (!schedule_file.empty()) {
        // Load from file
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
