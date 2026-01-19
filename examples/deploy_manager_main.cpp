#include "deploy_manager.h"
#include <iostream>
#include <signal.h>
#include <cstring>

using namespace orchestrator;

// Global deploy manager pointer for signal handling
DeployManager* g_deploy_manager = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[DeployManager] Received signal " << signal << ", cleaning up..." << std::endl;
    if (g_deploy_manager) {
        g_deploy_manager->cleanup_all();
    }
    exit(0);
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [COMMAND] [OPTIONS]" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  build                   Build Docker images" << std::endl;
    std::cout << "  deploy                  Deploy all containers (tasks + orchestrator)" << std::endl;
    std::cout << "  deploy-tasks            Deploy only task containers" << std::endl;
    std::cout << "  deploy-orchestrator     Deploy only orchestrator container" << std::endl;
    std::cout << "  start                   Start all containers" << std::endl;
    std::cout << "  stop                    Stop all containers" << std::endl;
    std::cout << "  cleanup                 Remove all containers and network" << std::endl;
    std::cout << "  status                  Show deployment status" << std::endl;
    std::cout << "  logs [container]        Follow logs (all containers or specific one)" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --config <file>         Deployment config file (default: deploy/deployment_config.yaml)" << std::endl;
    std::cout << "  --help                  Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << " build" << std::endl;
    std::cout << "  " << program_name << " deploy-tasks" << std::endl;
    std::cout << "  " << program_name << " deploy-orchestrator" << std::endl;
    std::cout << "  " << program_name << " logs" << std::endl;
    std::cout << "  " << program_name << " logs grpc_orchestrator" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== gRPC Deploy Manager ===" << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments
    std::string command;
    std::string config_file = "deploy/deployment_config.yaml";
    std::string container_name;  // For logs command
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (command.empty() && arg[0] != '-') {
            command = arg;
        } else if (!command.empty() && command == "logs" && arg[0] != '-') {
            container_name = arg;  // Optional container name for logs
        }
    }
    
    if (command.empty()) {
        std::cerr << "Error: No command specified" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    // Create deploy manager
    DeployManager deploy_manager;
    g_deploy_manager = &deploy_manager;
    
    // Load configuration
    if (!deploy_manager.load_config(config_file)) {
        std::cerr << "[DeployManager] Failed to load configuration" << std::endl;
        return 1;
    }
    
    // Execute command
    bool success = false;
    
    if (command == "build") {
        std::cout << "[DeployManager] Building images..." << std::endl;
        success = deploy_manager.build_images();
        
    } else if (command == "deploy") {
        std::cout << "[DeployManager] Deploying all containers (tasks + orchestrator)..." << std::endl;
        
        // Build images first
        if (!deploy_manager.build_images()) {
            std::cerr << "[DeployManager] Build failed, aborting deployment" << std::endl;
            return 1;
        }
        
        // Deploy all
        success = deploy_manager.deploy_all();
        
    } else if (command == "deploy-tasks") {
        std::cout << "[DeployManager] Deploying task containers only..." << std::endl;
        
        // Build images first
        if (!deploy_manager.build_images()) {
            std::cerr << "[DeployManager] Build failed, aborting deployment" << std::endl;
            return 1;
        }
        
        // Deploy tasks only
        success = deploy_manager.deploy_tasks_only();
        
    } else if (command == "deploy-orchestrator") {
        std::cout << "[DeployManager] Deploying orchestrator container..." << std::endl;
        
        // Check if network exists
        if (!deploy_manager.create_network()) {
            std::cerr << "[DeployManager] Failed to create/verify network" << std::endl;
            return 1;
        }
        
        // Deploy orchestrator only
        success = deploy_manager.deploy_orchestrator();
        
    } else if (command == "start") {
        std::cout << "[DeployManager] Starting containers..." << std::endl;
        // Start orchestrator
        success = deploy_manager.start_container(
            deploy_manager.get_config().orchestrator.container_name);
        
        // Start tasks
        for (const auto& task : deploy_manager.get_config().tasks) {
            deploy_manager.start_container(task.container_name);
        }
        
    } else if (command == "stop") {
        std::cout << "[DeployManager] Stopping containers..." << std::endl;
        // Stop orchestrator
        success = deploy_manager.stop_container(
            deploy_manager.get_config().orchestrator.container_name);
        
        // Stop tasks
        for (const auto& task : deploy_manager.get_config().tasks) {
            deploy_manager.stop_container(task.container_name);
        }
        
    } else if (command == "cleanup") {
        std::cout << "[DeployManager] Cleaning up..." << std::endl;
        success = deploy_manager.cleanup_all();
        
    } else if (command == "status") {
        std::cout << "[DeployManager] Deployment status:" << std::endl;
        deploy_manager.print_status();
        success = true;
        
    } else if (command == "logs") {
        deploy_manager.follow_logs(container_name);
        success = true;
        
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    if (success) {
        std::cout << "[DeployManager] Command '" << command << "' completed successfully" << std::endl;
        return 0;
    } else {
        std::cerr << "[DeployManager] Command '" << command << "' failed" << std::endl;
        return 1;
    }
}
