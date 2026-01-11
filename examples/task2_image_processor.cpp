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
    std::cout << "\n[Task 2 - Image Processor] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_task_wrapper) {
        g_task_wrapper->stop();
    }
    exit(0);
}

// ============================================================
// TASK 2: IMAGE PROCESSOR
// Simula elaborazione di immagini con filtri e trasformazioni
// Input: image_width, image_height, filter (optional)
// Output: pixels_processed, edges_detected, edge_density
// ============================================================
TaskResult image_processor_function(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 2 - Image Processor] Starting execution" << std::endl;
    
    // Parse input JSON
    json params;
    try {
        params = json::parse(params_json);
    } catch (const json::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] JSON parse error: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    if (!params.contains("image_width") || !params.contains("image_height")) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] ERROR: Missing image dimensions" << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    json output = json::object();
    
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
                  << "[Task 2 - Image Processor] ========================================" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] Processing complete!" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] Results:" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Image size: " << width << "x" << height << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Total pixels: " << total_pixels << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Edges detected: " << edge_count << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Edge density: " << (edge_count * 100.0 / total_pixels) << "%" << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "  - Filter applied: " << filter_type << std::endl;
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] ========================================" << std::endl;
        
        output["pixels_processed"] = total_pixels;
        output["edges_detected"] = edge_count;
        output["edge_density"] = edge_count * 100.0 / total_pixels;
        output["filter_applied"] = filter_type;
        
    } catch (const std::exception& e) {
        std::cerr << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 2 - Image Processor] ERROR: " << e.what() << std::endl;
        return TASK_RESULT_FAILURE;
    }
    
    // Convert output to JSON string
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 2 - Image Processor] Task completed successfully" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nTask 2 - Image Processor" << std::endl;
    std::cout << "Processes images with Gaussian blur and edge detection (Sobel operator)" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --name <id>             Task ID (should be 'task_2')" << std::endl;
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
    
    std::cout << "=== Task 2: Image Processor ===" << std::endl;
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
        image_processor_function
    );
    
    g_task_wrapper = &task_wrapper;
    
    // Set real-time configuration
    if (rt_config.policy != RT_POLICY_NONE) {
        std::cout << "[Task 2 - Image Processor] Configuring real-time scheduling" << std::endl;
        task_wrapper.set_rt_config(rt_config);
    } else {
        std::cout << "[Task 2 - Image Processor] Running in non-real-time mode" << std::endl;
    }
    
    // Start task wrapper
    task_wrapper.start();
    
    std::cout << "[Task 2 - Image Processor] Ready and waiting for commands..." << std::endl;
    std::cout << "[Task 2 - Image Processor] Press Ctrl+C to stop" << std::endl;
    
    // Keep running until stopped
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (task_wrapper.get_state() == TASK_STATE_STOPPED) {
            break;
        }
    }
    
    std::cout << "[Task 2 - Image Processor] Stopped" << std::endl;
    
    return 0;
}
