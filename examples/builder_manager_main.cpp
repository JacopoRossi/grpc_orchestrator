#include "builder_manager.h"
#include <iostream>
#include <signal.h>
#include <cstring>

using namespace orchestrator;

// Global builder manager pointer for signal handling
BuilderManager* g_builder_manager = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[BuilderManager] Received signal " << signal << ", exiting..." << std::endl;
    exit(0);
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [COMMAND] [OPTIONS]" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  build                   Build Docker images locally" << std::endl;
    std::cout << "  push                    Push images to GitLab registry" << std::endl;
    std::cout << "  build-push              Build and push images to GitLab (complete workflow)" << std::endl;
    std::cout << "  login                   Login to GitLab registry" << std::endl;
    std::cout << "  logout                  Logout from GitLab registry" << std::endl;
    std::cout << "  cleanup                 Remove local images" << std::endl;
    std::cout << "  status                  Show builder status" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --config <file>         Builder config file (default: deploy/builder_config.yaml)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << " build" << std::endl;
    std::cout << "  " << program_name << " push" << std::endl;
    std::cout << "  " << program_name << " build-push" << std::endl;
    std::cout << "  " << program_name << " status" << std::endl;
    std::cout << "\nEnvironment Variables:" << std::endl;
    std::cout << "  CI_JOB_TOKEN           GitLab CI/CD token (automatically set in CI pipelines)" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== gRPC Builder Manager ===" << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments
    std::string command;
    std::string config_file = "deploy/builder_config.yaml";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (command.empty() && arg[0] != '-') {
            command = arg;
        }
    }
    
    if (command.empty()) {
        std::cerr << "Error: No command specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    // Create builder manager
    BuilderManager builder_manager;
    g_builder_manager = &builder_manager;
    
    // Load configuration
    if (!builder_manager.load_config(config_file)) {
        std::cerr << "[BuilderManager] Failed to load configuration" << std::endl;
        return 1;
    }
    
    // Execute command
    bool success = false;
    
    if (command == "build") {
        std::cout << "[BuilderManager] Building images..." << std::endl;
        success = builder_manager.build_all_images();
        
    } else if (command == "push") {
        std::cout << "[BuilderManager] Pushing images to GitLab..." << std::endl;
        success = builder_manager.push_all_to_gitlab();
        
    } else if (command == "build-push") {
        std::cout << "[BuilderManager] Building and pushing images..." << std::endl;
        success = builder_manager.build_and_push_all();
        
    } else if (command == "login") {
        std::cout << "[BuilderManager] Logging in to GitLab..." << std::endl;
        success = builder_manager.login_to_gitlab();
        
    } else if (command == "logout") {
        std::cout << "[BuilderManager] Logging out from GitLab..." << std::endl;
        success = builder_manager.logout_from_gitlab();
        
    } else if (command == "cleanup") {
        std::cout << "[BuilderManager] Cleaning up local images..." << std::endl;
        success = builder_manager.cleanup_local_images();
        
    } else if (command == "status") {
        std::cout << "[BuilderManager] Builder status:" << std::endl;
        builder_manager.print_status();
        success = true;
        
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    if (success) {
        std::cout << "[BuilderManager] Command '" << command << "' completed successfully" << std::endl;
        return 0;
    } else {
        std::cerr << "[BuilderManager] Command '" << command << "' failed" << std::endl;
        return 1;
    }
}
