#include "task_wrapper.h"
#include "rt_utils.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <signal.h>
#include <cstring>
#include <vector>
#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orchestrator;

// Global task wrapper pointer for signal handling
TaskWrapper* g_task_wrapper = nullptr;

// Helper function to get absolute timestamp in milliseconds
int64_t get_absolute_time_ms() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now;
}

void signal_handler(int signal) {
    std::cout << "\n[Task 1 - Data Analyzer] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

// ============================================================
// TASK 1: DATA ANALYZER
// Analizza un array di dati numerici e calcola statistiche
// Input: data_size (numero di campioni)
// Output: media, min, max, deviazione standard
// ============================================================
TaskResult data_analyzer_function(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 1 - Data Analyzer] Starting execution" << std::endl;
    
    // Parse input JSON
    json params;
    try {
        params = json::parse(params_json);
    } catch (const json::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] JSON parse error: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    if (!params.contains("data_size")) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] ERROR: Missing 'data_size' parameter" << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    json output = json::object();
    
    try {
        int data_size = params["data_size"].is_string() ? 
            std::stoi(params["data_size"].get<std::string>()) : 
            params["data_size"].get<int>();
        
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] Starting analysis of " << data_size << " data points" << std::endl;
        
        // Genera dati simulati (in un caso reale sarebbero letti da file/database)
        std::vector<double> data(data_size);
        for (int i = 0; i < data_size; i++) {
            data[i] = 100.0 + (i % 50) * 2.5 + (i % 7) * 0.3;
        }
        
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] Data generated, computing statistics..." << std::endl;
        
        // Calcola statistiche con carico computazionale realistico
        double sum = 0.0, min_val = data[0], max_val = data[0];
        
        // Prima passata: somma, min, max
        for (int i = 0; i < data_size; i++) {
            sum += data[i];
            if (data[i] < min_val) min_val = data[i];
            if (data[i] > max_val) max_val = data[i];
            
            // Simula elaborazione pesante ogni 100k elementi
            if (i > 0 && i % 100000 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                          << "[Task 1 - Data Analyzer] Processed " << i << "/" << data_size << " elements" << std::endl;
            }
        }
        
        double mean = sum / data_size;
        
        // Seconda passata: deviazione standard
        double variance_sum = 0.0;
        for (int i = 0; i < data_size; i++) {
            double diff = data[i] - mean;
            variance_sum += diff * diff;
            
            if (i > 0 && i % 100000 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        
        double std_dev = std::sqrt(variance_sum / data_size);
        
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] ========================================" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] Analysis complete!" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] Results:" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Samples: " << data_size << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Mean: " << mean << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Min: " << min_val << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Max: " << max_val << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Std Dev: " << std_dev << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] ========================================" << std::endl;
        
        output["mean"] = mean;
        output["min"] = min_val;
        output["max"] = max_val;
        output["std_dev"] = std_dev;
        output["samples_analyzed"] = data_size;
        
    } catch (const std::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 1 - Data Analyzer] ERROR: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    // Convert output to JSON string
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 1 - Data Analyzer] Task completed successfully" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nTask 1 - Data Analyzer" << std::endl;
    std::cout << "Analyzes numerical data and computes statistics (mean, min, max, std dev)" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --name <id>             Task ID (should be 'task_1')" << std::endl;
    std::cout << "  --address <addr>        Listen address" << std::endl;
    std::cout << "  --orchestrator <addr>   Orchestrator address" << std::endl;
    std::cout << "\nReal-Time Options:" << std::endl;
    std::cout << "  --policy <policy>       RT scheduling policy: none, fifo, rr (default: none)" << std::endl;
    std::cout << "  --priority <n>          RT priority: 1-99 (default: 50)" << std::endl;
    std::cout << "  --cpu-affinity <n>      Bind to CPU core (default: -1, no affinity)" << std::endl;
    std::cout << "  --lock-memory           Lock memory pages (prevents page faults)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
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
    
    std::cout << "=== Task 1: Data Analyzer ===" << std::endl;
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
        data_analyzer_function
    );
    
    g_task_wrapper = &task_wrapper;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Task 1 - Data Analyzer] Configuring real-time scheduling" << std::endl;
        task_wrapper.set_rt_config(rt_config);
    } else {
        std::cout << "[Task 1 - Data Analyzer] Running in non-real-time mode" << std::endl;
    }
    
    // Start task wrapper
    task_wrapper.start();
    
    std::cout << "[Task 1 - Data Analyzer] Ready and waiting for commands..." << std::endl;
    std::cout << "[Task 1 - Data Analyzer] Press Ctrl+C to stop" << std::endl;
    
    // Keep running until stopped
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;
        }
    }
    
    std::cout << "[Task 1 - Data Analyzer] Stopped" << std::endl;
    
    return 0;
}
