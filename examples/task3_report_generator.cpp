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
    std::cout << "\n[Task 3 - Report Generator] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

// ============================================================
// TASK 3: REPORT GENERATOR
// Prende i risultati dell'analisi dati (Task 1) e genera un report
// con elaborazioni aggiuntive pesanti (simulazione ML, aggregazioni)
// Input: dep_output (from Task 1), report_depth (optional)
// Output: report completo con metriche avanzate
// ============================================================
TaskResult report_generator_function(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 3 - Report Generator] Starting execution" << std::endl;
    
    // Parse input JSON
    json params;
    try {
        params = json::parse(params_json);
    } catch (const json::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] JSON parse error: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    if (!params.contains("dep_output")) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] ERROR: Missing dependency output from Task 1" << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    json output = json::object();
    
    try {
        json dep_output = params["dep_output"];
        
        if (!dep_output.contains("mean") || !dep_output.contains("std_dev")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3 - Report Generator] ERROR: Invalid data from Task 1" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        double mean = dep_output["mean"].get<double>();
        double min_val = dep_output["min"].get<double>();
        double max_val = dep_output["max"].get<double>();
        double std_dev = dep_output["std_dev"].get<double>();
        int samples = dep_output["samples_analyzed"].get<int>();
        
        int report_depth = params.value("report_depth", 3);
        
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Generating comprehensive report..." << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Input data: " << samples << " samples from Task 1" << std::endl;
        
        // Fase 1: Calcolo metriche avanzate
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Phase 1/4: Computing advanced metrics..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        double range = max_val - min_val;
        double cv = (std_dev / mean) * 100.0; // Coefficient of variation
        double z_score_max = (max_val - mean) / std_dev;
        double z_score_min = (min_val - mean) / std_dev;
        
        // Fase 2: Simulazione analisi predittiva (ML)
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Phase 2/4: Running predictive analysis (ML simulation)..." << std::endl;
        
        double prediction_accuracy = 0.0;
        for (int i = 0; i < 100; i++) {
            // Simula training di modello ML
            double error = std::abs(std::sin(i * 0.1)) * std_dev;
            prediction_accuracy += (1.0 - error / range);
            
            if (i % 20 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                          << "[Task 3 - Report Generator] ML training progress: " << i << "%" << std::endl;
            }
        }
        prediction_accuracy /= 100.0;
        
        // Fase 3: Generazione aggregazioni temporali
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Phase 3/4: Computing temporal aggregations..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::vector<double> moving_averages;
        int window_size = std::min(100, samples / 10);
        for (int i = 0; i < 50; i++) {
            double ma = mean + (std::sin(i * 0.2) * std_dev * 0.5);
            moving_averages.push_back(ma);
        }
        
        // Fase 4: Generazione visualizzazioni e export
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Phase 4/4: Generating visualizations and exporting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Simula generazione grafici
        for (int i = 0; i < report_depth; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3 - Report Generator] Generated chart " << (i+1) << "/" << report_depth << std::endl;
        }
        
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] ============================================" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] COMPREHENSIVE ANALYSIS REPORT" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] ============================================" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Basic Statistics:" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Samples: " << samples << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Mean: " << mean << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Range: " << range << " (Min: " << min_val << ", Max: " << max_val << ")" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Std Dev: " << std_dev << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] Advanced Metrics:" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Coefficient of Variation: " << cv << "%" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Z-Score Range: [" << z_score_min << ", " << z_score_max << "]" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Prediction Accuracy: " << (prediction_accuracy * 100) << "%" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Moving Averages Computed: " << moving_averages.size() << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Charts Generated: " << report_depth << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] ============================================" << std::endl;
        
        output["samples"] = samples;
        output["mean"] = mean;
        output["range"] = range;
        output["std_dev"] = std_dev;
        output["coefficient_of_variation"] = cv;
        output["z_score_min"] = z_score_min;
        output["z_score_max"] = z_score_max;
        output["prediction_accuracy"] = prediction_accuracy;
        output["moving_averages_count"] = static_cast<int>(moving_averages.size());
        output["charts_generated"] = report_depth;
        output["report_status"] = "complete";
        
    } catch (const std::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3 - Report Generator] ERROR: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    // Convert output to JSON string
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 3 - Report Generator] Task completed successfully" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nTask 3 - Report Generator" << std::endl;
    std::cout << "Generates comprehensive reports from Task 1 data analysis results" << std::endl;
    std::cout << "Includes ML simulation, temporal aggregations, and visualizations" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --name <id>             Task ID (should be 'task_3')" << std::endl;
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
    
    std::cout << "=== Task 3: Report Generator ===" << std::endl;
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
        report_generator_function
    );
    
    g_task_wrapper = &task_wrapper;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Task 3 - Report Generator] Configuring real-time scheduling" << std::endl;
        task_wrapper.set_rt_config(rt_config);
    } else {
        std::cout << "[Task 3 - Report Generator] Running in non-real-time mode" << std::endl;
    }
    
    // Start task wrapper
    task_wrapper.start();
    
    std::cout << "[Task 3 - Report Generator] Ready and waiting for commands..." << std::endl;
    std::cout << "[Task 3 - Report Generator] Press Ctrl+C to stop" << std::endl;
    
    // Keep running until stopped
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;
        }
    }
    
    std::cout << "[Task 3 - Report Generator] Stopped" << std::endl;
    
    return 0;
}
