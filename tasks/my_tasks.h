#pragma once

#include "task_wrapper.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>

using json = nlohmann::json;
using namespace orchestrator;

// Helper function per timestamp
inline int64_t get_absolute_time_ms() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now;
}

// ============================================================
// TASK 1: DATA ANALYZER
// Analizza dati e calcola statistiche
// Input: data_size (numero di campioni)
// Output: media, min, max, deviazione standard
// ============================================================
inline TaskResult task1_data_analyzer(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 1] Starting data analysis" << std::endl;
    
    // Parse input
    json params = json::parse(params_json);
    int data_size = params.value("data_size", 1000);
    
    // Genera e analizza dati
    std::vector<double> data(data_size);
    for (int i = 0; i < data_size; i++) {
        data[i] = 100.0 + (i % 50) * 2.5;
    }
    
    // Calcola statistiche
    double sum = 0.0, min_val = data[0], max_val = data[0];
    for (const auto& val : data) {
        sum += val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }
    double mean = sum / data_size;
    
    // Deviazione standard
    double variance_sum = 0.0;
    for (const auto& val : data) {
        double diff = val - mean;
        variance_sum += diff * diff;
    }
    double std_dev = std::sqrt(variance_sum / data_size);
    
    // Output
    json output;
    output["mean"] = mean;
    output["min"] = min_val;
    output["max"] = max_val;
    output["std_dev"] = std_dev;
    output["samples"] = data_size;
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 1] Analysis complete: mean=" << mean << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

// ============================================================
// TASK 2: IMAGE PROCESSOR
// Elabora immagini con filtri
// Input: image_width, image_height, filter
// Output: processed_pixels, filter_applied
// ============================================================
inline TaskResult task2_image_processor(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 2] Starting image processing" << std::endl;
    
    // Parse input
    json params = json::parse(params_json);
    int width = params.value("image_width", 800);
    int height = params.value("image_height", 600);
    std::string filter = params.value("filter", "gaussian");
    
    // Simula elaborazione immagine
    int total_pixels = width * height;
    std::vector<uint8_t> image(total_pixels);
    
    // Applica filtro (simulato)
    for (int i = 0; i < total_pixels; i++) {
        image[i] = (i * 7) % 256;
    }
    
    // Simula carico computazionale
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Output
    json output;
    output["processed_pixels"] = total_pixels;
    output["filter_applied"] = filter;
    output["width"] = width;
    output["height"] = height;
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 2] Processing complete: " << total_pixels << " pixels" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

// ============================================================
// TASK 3: REPORT GENERATOR
// Genera report dai risultati di altri task
// Input: report_depth, dep_output (da task precedente)
// Output: report_id, charts_generated
// ============================================================
inline TaskResult task3_report_generator(const std::string& params_json, std::string& output_json) {
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 3] Starting report generation" << std::endl;
    
    // Parse input
    json params = json::parse(params_json);
    int depth = params.value("report_depth", 5);
    
    // Controlla se ci sono dati da task precedente
    if (params.contains("dep_output")) {
        json dep_data = params["dep_output"];
        std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
                  << "[Task 3] Using dependency output: " << dep_data.dump() << std::endl;
    }
    
    // Simula generazione report
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Output
    json output;
    output["report_id"] = "RPT-" + std::to_string(get_absolute_time_ms());
    output["charts_generated"] = depth;
    output["status"] = "complete";
    output_json = output.dump();
    
    std::cout << "[" << std::setw(13) << get_absolute_time_ms() << " ms] "
              << "[Task 3] Report complete with " << depth << " charts" << std::endl;
    
    return TASK_RESULT_SUCCESS;
}

// ============================================================
// ESEMPIO: TASK PERSONALIZZATO
// Qui puoi aggiungere i tuoi task personalizzati
// ============================================================
inline TaskResult my_custom_task(const std::string& params_json, std::string& output_json) {
    std::cout << "[Custom Task] Starting..." << std::endl;
    
    // Parse parametri
    json params = json::parse(params_json);
    
    // TODO: Inserisci qui la tua logica
    // Esempio:
    // int iterations = params.value("iterations", 100);
    // for (int i = 0; i < iterations; i++) {
    //     // fai qualcosa
    // }
    
    // Prepara output
    json output;
    output["status"] = "completed";
    output["message"] = "Custom task executed successfully";
    output_json = output.dump();
    
    std::cout << "[Custom Task] Completed" << std::endl;
    return TASK_RESULT_SUCCESS;
}
