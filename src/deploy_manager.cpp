#include "deploy_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <array>
#include <memory>

namespace orchestrator {

DeployManager::DeployManager() : config_loaded_(false) {
}

DeployManager::~DeployManager() {
}

bool DeployManager::load_config(const std::string& config_path) {
    std::cout << "[DeployManager] Loading configuration from: " << config_path << std::endl;
    
    try {
        config_ = DeploymentConfigParser::parse_yaml(config_path);
        config_loaded_ = true;
        
        std::cout << "[DeployManager] Configuration loaded successfully" << std::endl;
        std::cout << "  Deployment: " << config_.name << std::endl;
        std::cout << "  Orchestrator: " << config_.orchestrator.container_name << std::endl;
        std::cout << "  Tasks: " << config_.tasks.size() << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DeployManager] Failed to load configuration: " << e.what() << std::endl;
        return false;
    }
}

std::string DeployManager::execute_command(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

bool DeployManager::execute_command_bool(const std::string& command) {
    int ret = system(command.c_str());
    return ret == 0;
}

bool DeployManager::build_images() {
    if (!config_loaded_) {
        std::cerr << "[DeployManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[DeployManager] Building Docker images..." << std::endl;
    
    // Build orchestrator image
    std::cout << "[DeployManager] Building orchestrator image..." << std::endl;
    if (!build_image(config_.orchestrator.dockerfile, config_.orchestrator.image)) {
        std::cerr << "[DeployManager] Failed to build orchestrator image" << std::endl;
        return false;
    }
    
    // Build task image (shared by all tasks)
    if (!config_.tasks.empty()) {
        std::cout << "[DeployManager] Building task image..." << std::endl;
        if (!build_image(config_.tasks[0].dockerfile, config_.tasks[0].image)) {
            std::cerr << "[DeployManager] Failed to build task image" << std::endl;
            return false;
        }
    }
    
    std::cout << "[DeployManager] All images built successfully" << std::endl;
    return true;
}

bool DeployManager::build_image(const std::string& dockerfile, const std::string& tag) {
    std::string cache_flag = config_.build_no_cache ? "--no-cache" : "";
    std::string command = "docker build " + cache_flag + " -f " + dockerfile + 
                         " -t " + tag + " " + config_.build_context;
    
    std::cout << "[DeployManager] Executing: " << command << std::endl;
    return execute_command_bool(command);
}

bool DeployManager::create_network() {
    std::cout << "[DeployManager] Creating network: " << config_.network_name << std::endl;
    
    // Check if network already exists
    std::string check_cmd = "docker network ls --filter name=" + config_.network_name + 
                           " --format '{{.Name}}' | grep -w " + config_.network_name;
    std::string result = execute_command(check_cmd);
    
    if (!result.empty()) {
        std::cout << "[DeployManager] Network already exists" << std::endl;
        return true;
    }
    
    // Create network
    std::string create_cmd = "docker network create --driver " + config_.network_driver + 
                            " " + config_.network_name;
    return execute_command_bool(create_cmd);
}

bool DeployManager::remove_network() {
    std::cout << "[DeployManager] Removing network: " << config_.network_name << std::endl;
    std::string command = "docker network rm " + config_.network_name + " 2>/dev/null || true";
    return execute_command_bool(command);
}

std::string DeployManager::generate_orchestrator_run_command() {
    std::stringstream cmd;
    
    cmd << "docker run -d";
    cmd << " --name " << config_.orchestrator.container_name;
    cmd << " --hostname " << config_.orchestrator.hostname;
    cmd << " --network " << config_.network_name;
    cmd << " -p 50050:50050";
    
    // Capabilities
    for (const auto& cap : config_.orchestrator.capabilities) {
        cmd << " --cap-add=" << cap;
    }
    
    // CPU set
    if (!config_.orchestrator.cpuset.empty()) {
        cmd << " --cpuset-cpus=" << config_.orchestrator.cpuset;
    }
    
    // Memory lock
    cmd << " --ulimit memlock=" << config_.orchestrator.memlock;
    cmd << " --ulimit rtprio=99";
    cmd << " --ulimit rttime=-1";
    
    // Security
    cmd << " --security-opt seccomp:unconfined";
    
    // Environment
    cmd << " -e DOCKER_CONTAINER=true";
    cmd << " -e TZ=UTC";
    
    // Image
    cmd << " " << config_.orchestrator.image;
    
    // Command
    cmd << " ./orchestrator_main";
    cmd << " --address " << config_.orchestrator.address;
    cmd << " --schedule " << config_.orchestrator.schedule_file;
    
    if (config_.orchestrator.rt_policy != "none") {
        cmd << " --policy " << config_.orchestrator.rt_policy;
        cmd << " --priority " << config_.orchestrator.rt_priority;
        cmd << " --cpu-affinity " << config_.orchestrator.rt_cpu_affinity;
        if (config_.orchestrator.lock_memory) {
            cmd << " --lock-memory";
        }
    }
    
    return cmd.str();
}

std::string DeployManager::generate_docker_run_command(const ContainerConfig& config) {
    std::stringstream cmd;
    
    cmd << "docker run -d";
    cmd << " --name " << config.container_name;
    cmd << " --hostname " << config.hostname;
    cmd << " --network " << config_.network_name;
    cmd << " -p " << config.port << ":" << config.port;
    
    // Capabilities
    for (const auto& cap : config.capabilities) {
        cmd << " --cap-add=" << cap;
    }
    
    // CPU set (if specified)
    if (!config.cpuset.empty()) {
        cmd << " --cpuset-cpus=" << config.cpuset;
    }
    
    // Memory lock
    cmd << " --ulimit memlock=" << config.memlock;
    cmd << " --ulimit rtprio=99";
    cmd << " --ulimit rttime=-1";
    
    // Security
    cmd << " --security-opt seccomp:unconfined";
    
    // Environment
    cmd << " -e TASK_ID=" << config.id;
    cmd << " -e DOCKER_CONTAINER=true";
    cmd << " -e TZ=UTC";
    
    // Image
    cmd << " " << config.image;
    
    // Command
    cmd << " ./task_runner";
    cmd << " --name " << config.task_type;
    cmd << " --address " << config.address;
    cmd << " --orchestrator " << config_.orchestrator.hostname << ":50050";
    
    return cmd.str();
}

bool DeployManager::deploy_orchestrator() {
    std::cout << "[DeployManager] Deploying orchestrator..." << std::endl;
    
    // Remove existing container if present
    remove_container(config_.orchestrator.container_name);
    
    // Generate and execute run command
    std::string command = generate_orchestrator_run_command();
    std::cout << "[DeployManager] Executing: " << command << std::endl;
    
    if (!execute_command_bool(command)) {
        std::cerr << "[DeployManager] Failed to deploy orchestrator" << std::endl;
        return false;
    }
    
    // Wait for healthy
    if (config_.wait_for_healthy) {
        std::cout << "[DeployManager] Waiting for orchestrator to be healthy..." << std::endl;
        if (!wait_for_healthy(config_.orchestrator.container_name, config_.startup_timeout)) {
            std::cerr << "[DeployManager] Orchestrator failed health check" << std::endl;
            return false;
        }
    }
    
    std::cout << "[DeployManager] Orchestrator deployed successfully" << std::endl;
    return true;
}

bool DeployManager::deploy_task(const ContainerConfig& task_config) {
    std::cout << "[DeployManager] Deploying task: " << task_config.id << std::endl;
    
    // Remove existing container if present
    remove_container(task_config.container_name);
    
    // Generate and execute run command
    std::string command = generate_docker_run_command(task_config);
    std::cout << "[DeployManager] Executing: " << command << std::endl;
    
    if (!execute_command_bool(command)) {
        std::cerr << "[DeployManager] Failed to deploy task: " << task_config.id << std::endl;
        return false;
    }
    
    // Wait for healthy
    if (config_.wait_for_healthy) {
        std::cout << "[DeployManager] Waiting for task to be healthy..." << std::endl;
        if (!wait_for_healthy(task_config.container_name, config_.startup_timeout)) {
            std::cerr << "[DeployManager] Task failed health check: " << task_config.id << std::endl;
            return false;
        }
    }
    
    std::cout << "[DeployManager] Task deployed successfully: " << task_config.id << std::endl;
    return true;
}

bool DeployManager::deploy_all() {
    if (!config_loaded_) {
        std::cerr << "[DeployManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[DeployManager] Starting full deployment (tasks + orchestrator)..." << std::endl;
    
    // Create network
    if (!create_network()) {
        std::cerr << "[DeployManager] Failed to create network" << std::endl;
        return false;
    }
    
    // Deploy tasks first (orchestrator depends on them)
    for (const auto& task : config_.tasks) {
        if (!deploy_task(task)) {
            std::cerr << "[DeployManager] Deployment failed at task: " << task.id << std::endl;
            return false;
        }
        
        // Small delay between deployments
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // Deploy orchestrator last
    if (!deploy_orchestrator()) {
        std::cerr << "[DeployManager] Failed to deploy orchestrator" << std::endl;
        return false;
    }
    
    std::cout << "[DeployManager] Full deployment completed successfully!" << std::endl;
    print_status();
    
    return true;
}

bool DeployManager::deploy_tasks_only() {
    if (!config_loaded_) {
        std::cerr << "[DeployManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[DeployManager] Starting task deployment (tasks only)..." << std::endl;
    
    // Create network
    if (!create_network()) {
        std::cerr << "[DeployManager] Failed to create network" << std::endl;
        return false;
    }
    
    // Deploy tasks only
    for (const auto& task : config_.tasks) {
        if (!deploy_task(task)) {
            std::cerr << "[DeployManager] Deployment failed at task: " << task.id << std::endl;
            return false;
        }
        
        // Small delay between deployments
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    std::cout << "[DeployManager] Task deployment completed successfully!" << std::endl;
    std::cout << "[DeployManager] Tasks are ready. Use 'deploy-orchestrator' to start the orchestrator when needed." << std::endl;
    print_status();
    
    return true;
}

bool DeployManager::start_container(const std::string& container_name) {
    std::string command = "docker start " + container_name;
    return execute_command_bool(command);
}

bool DeployManager::stop_container(const std::string& container_name) {
    std::string command = "docker stop " + container_name + " 2>/dev/null || true";
    return execute_command_bool(command);
}

bool DeployManager::remove_container(const std::string& container_name) {
    std::string command = "docker rm -f " + container_name + " 2>/dev/null || true";
    return execute_command_bool(command);
}

bool DeployManager::wait_for_healthy(const std::string& container_name, int timeout_seconds) {
    for (int i = 0; i < timeout_seconds; i++) {
        if (is_container_healthy(container_name)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return false;
}

bool DeployManager::is_container_healthy(const std::string& container_name) {
    std::string command = "docker inspect --format='{{.State.Running}}' " + container_name + " 2>/dev/null";
    std::string result = execute_command(command);
    return result.find("true") != std::string::npos;
}

bool DeployManager::cleanup_all() {
    std::cout << "[DeployManager] Cleaning up deployment..." << std::endl;
    
    // Stop and remove orchestrator
    stop_container(config_.orchestrator.container_name);
    remove_container(config_.orchestrator.container_name);
    
    // Stop and remove all tasks
    for (const auto& task : config_.tasks) {
        stop_container(task.container_name);
        remove_container(task.container_name);
    }
    
    // Remove network
    remove_network();
    
    std::cout << "[DeployManager] Cleanup completed" << std::endl;
    return true;
}

void DeployManager::print_status() {
    std::cout << "\n=== Deployment Status ===" << std::endl;
    std::cout << "Network: " << config_.network_name << std::endl;
    
    auto containers = get_running_containers();
    std::cout << "Running containers: " << containers.size() << std::endl;
    for (const auto& container : containers) {
        std::cout << "  - " << container << std::endl;
    }
}

std::vector<std::string> DeployManager::get_running_containers() {
    std::vector<std::string> containers;
    
    std::string command = "docker ps --filter network=" + config_.network_name + 
                         " --format '{{.Names}}'";
    std::string result = execute_command(command);
    
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            containers.push_back(line);
        }
    }
    
    return containers;
}

// Parser implementation
DeploymentConfig DeploymentConfigParser::parse_yaml(const std::string& yaml_path) {
    YAML::Node config = YAML::LoadFile(yaml_path);
    DeploymentConfig deployment;
    
    YAML::Node deploy = config["deployment"];
    
    // Basic info
    deployment.name = deploy["name"].as<std::string>();
    deployment.description = deploy["description"].as<std::string>();
    
    // Network
    YAML::Node network = deploy["network"];
    deployment.network_name = network["name"].as<std::string>();
    deployment.network_driver = network["driver"].as<std::string>();
    
    // Orchestrator
    YAML::Node orch = deploy["orchestrator"];
    deployment.orchestrator.container_name = orch["container_name"].as<std::string>();
    deployment.orchestrator.hostname = orch["hostname"].as<std::string>();
    deployment.orchestrator.image = orch["image"].as<std::string>();
    deployment.orchestrator.dockerfile = orch["dockerfile"].as<std::string>();
    deployment.orchestrator.address = orch["address"].as<std::string>();
    deployment.orchestrator.schedule_file = orch["schedule_file"].as<std::string>();
    
    // Orchestrator RT config
    if (orch["rt_config"]) {
        YAML::Node rt = orch["rt_config"];
        deployment.orchestrator.rt_policy = rt["policy"].as<std::string>();
        deployment.orchestrator.rt_priority = rt["priority"].as<int>();
        deployment.orchestrator.rt_cpu_affinity = rt["cpu_affinity"].as<int>();
        deployment.orchestrator.lock_memory = rt["lock_memory"].as<bool>();
    }
    
    // Orchestrator resources
    if (orch["resources"]) {
        YAML::Node res = orch["resources"];
        deployment.orchestrator.cpuset = res["cpuset"].as<std::string>();
        deployment.orchestrator.memlock = res["memlock"].as<int64_t>();
    }
    
    // Orchestrator capabilities
    if (orch["capabilities"]) {
        for (const auto& cap : orch["capabilities"]) {
            deployment.orchestrator.capabilities.push_back(cap.as<std::string>());
        }
    }
    
    // Orchestrator healthcheck
    if (orch["healthcheck"]) {
        YAML::Node hc = orch["healthcheck"];
        deployment.orchestrator.healthcheck_command = hc["command"].as<std::string>();
        deployment.orchestrator.healthcheck_interval = hc["interval"].as<int>();
        deployment.orchestrator.healthcheck_timeout = hc["timeout"].as<int>();
        deployment.orchestrator.healthcheck_retries = hc["retries"].as<int>();
    }
    
    // Tasks
    YAML::Node tasks = deploy["tasks"];
    for (const auto& task_node : tasks) {
        ContainerConfig task;
        
        task.id = task_node["id"].as<std::string>();
        task.container_name = task_node["container_name"].as<std::string>();
        task.hostname = task_node["hostname"].as<std::string>();
        task.image = task_node["image"].as<std::string>();
        task.dockerfile = task_node["dockerfile"].as<std::string>();
        task.address = task_node["address"].as<std::string>();
        task.port = task_node["port"].as<int>();
        task.task_type = task_node["task_type"].as<std::string>();
        
        // RT config
        if (task_node["rt_config"]) {
            YAML::Node rt = task_node["rt_config"];
            task.rt_policy = rt["policy"].as<std::string>();
            task.rt_priority = rt["priority"].as<int>();
            task.rt_cpu_affinity = rt["cpu_affinity"].as<int>();
        }
        
        // Resources
        if (task_node["resources"]) {
            YAML::Node res = task_node["resources"];
            if (res["cpuset"]) {
                task.cpuset = res["cpuset"].as<std::string>();
            }
            task.memlock = res["memlock"].as<int64_t>();
        }
        
        // Capabilities
        if (task_node["capabilities"]) {
            for (const auto& cap : task_node["capabilities"]) {
                task.capabilities.push_back(cap.as<std::string>());
            }
        }
        
        // Healthcheck
        if (task_node["healthcheck"]) {
            YAML::Node hc = task_node["healthcheck"];
            task.healthcheck_command = hc["command"].as<std::string>();
            task.healthcheck_interval = hc["interval"].as<int>();
            task.healthcheck_timeout = hc["timeout"].as<int>();
            task.healthcheck_retries = hc["retries"].as<int>();
        }
        
        deployment.tasks.push_back(task);
    }
    
    // Build configuration
    YAML::Node build = deploy["build"];
    deployment.build_context = build["context"].as<std::string>();
    deployment.build_parallel = build["parallel"].as<bool>();
    deployment.build_no_cache = build["no_cache"].as<bool>();
    
    // Strategy
    YAML::Node strategy = deploy["strategy"];
    deployment.strategy_type = strategy["type"].as<std::string>();
    deployment.wait_for_healthy = strategy["wait_for_healthy"].as<bool>();
    deployment.startup_timeout = strategy["startup_timeout"].as<int>();
    
    return deployment;
}

} // namespace orchestrator
