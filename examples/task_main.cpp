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
        // ============================================================
        // TASK 1: DATA ANALYZER
        // Analizza un array di dati numerici e calcola statistiche
        // Input: array di numeri, numero di campioni
        // Output: media, min, max, deviazione standard
        // ============================================================
        
        if (!params.contains("data_size")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1 - Data Analyzer] ERROR: Missing 'data_size' parameter" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
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
                      << "[Task 1 - Data Analyzer] Analysis complete!" << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 1 - Data Analyzer] Results:" << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Mean: " << mean << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Min: " << min_val << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Max: " << max_val << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Std Dev: " << std_dev << std::endl;
            
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
        
    } else if (task_id == "task_2") {
        // ============================================================
        // TASK 2: IMAGE PROCESSOR
        // Simula elaborazione di immagini con filtri e trasformazioni
        // Input: dimensioni immagine, tipo di filtro
        // Output: immagine processata (simulata), tempo di elaborazione
        // ============================================================
        
        if (!params.contains("image_width") || !params.contains("image_height")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] ERROR: Missing image dimensions" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
        try {
            int width = params["image_width"].is_string() ? 
                std::stoi(params["image_width"].get<std::string>()) : 
                params["image_width"].get<int>();
            int height = params["image_height"].is_string() ? 
                std::stoi(params["image_height"].get<std::string>()) : 
                params["image_height"].get<int>();
            
            std::string filter_type = params.value("filter", "gaussian");
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] Processing " << width << "x" << height 
                      << " image with " << filter_type << " filter" << std::endl;
            
            int total_pixels = width * height;
            
            // Simula caricamento immagine
            std::vector<std::vector<int>> image(height, std::vector<int>(width));
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    image[y][x] = (x + y) % 256;
                }
            }
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] Image loaded, applying filters..." << std::endl;
            
            // Applica filtro Gaussian blur (3x3 kernel)
            std::vector<std::vector<int>> processed(height, std::vector<int>(width));
            int processed_pixels = 0;
            
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    // Convoluzione 3x3
                    int sum = 0;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            sum += image[y + dy][x + dx];
                        }
                    }
                    processed[y][x] = sum / 9;
                    processed_pixels++;
                    
                    // Simula carico computazionale
                    if (processed_pixels % 50000 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                                  << "[Task 2 - Image Processor] Processed " 
                                  << (processed_pixels * 100 / total_pixels) << "% of pixels" << std::endl;
                    }
                }
            }
            
            // Applica edge detection (Sobel operator)
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] Applying edge detection..." << std::endl;
            
            int edge_count = 0;
            for (int y = 1; y < height - 1; y++) {
                for (int x = 1; x < width - 1; x++) {
                    int gx = -processed[y-1][x-1] + processed[y-1][x+1] 
                            -2*processed[y][x-1] + 2*processed[y][x+1]
                            -processed[y+1][x-1] + processed[y+1][x+1];
                    int gy = -processed[y-1][x-1] - 2*processed[y-1][x] - processed[y-1][x+1]
                            +processed[y+1][x-1] + 2*processed[y+1][x] + processed[y+1][x+1];
                    
                    int gradient = std::abs(gx) + std::abs(gy);
                    if (gradient > 128) edge_count++;
                }
                
                if (y % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] Processing complete!" << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] Results:" << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Total pixels: " << total_pixels << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Edges detected: " << edge_count << std::endl;
            std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "  - Edge density: " << (edge_count * 100.0 / total_pixels) << "%" << std::endl;
            
            output["pixels_processed"] = total_pixels;
            output["edges_detected"] = edge_count;
            output["edge_density"] = edge_count * 100.0 / total_pixels;
            output["filter_applied"] = filter_type;
            
        } catch (const std::exception& e) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 2 - Image Processor] ERROR: " << e.what() << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
    } else if (task_id == "task_3") {
        // ============================================================
        // TASK 3: REPORT GENERATOR
        // Prende i risultati dell'analisi dati (Task 1) e genera un report
        // con elaborazioni aggiuntive pesanti (simulazione ML, aggregazioni)
        // Input: risultati da Task 1, parametri di configurazione
        // Output: report completo con metriche avanzate
        // ============================================================
        
        if (!params.contains("dep_output")) {
            std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                      << "[Task 3 - Report Generator] ERROR: Missing dependency output from Task 1" << std::endl;
            return TASK_RESULT_FAILURE;
        }
        
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
                      << "[Task 3 - Report Generator] Input data: " << samples << " samples" << std::endl;
            
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
