#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace orchestrator {

// Container configuration
struct ContainerConfig {
    std::string id;
    std::string container_name;
    std::string hostname;
    std::string image;
    std::string dockerfile;
    std::string address;
    int port;
    std::string task_type;
    
    // RT configuration
    std::string rt_policy;
    int rt_priority;
    int rt_cpu_affinity;
    
    // Resources
    std::string cpuset;
    int64_t memlock;
    
    // Capabilities
    std::vector<std::string> capabilities;
    
    // Health check
    std::string healthcheck_command;
    int healthcheck_interval;
    int healthcheck_timeout;
    int healthcheck_retries;
};

// Orchestrator configuration
struct OrchestratorConfig {
    std::string container_name;
    std::string hostname;
    std::string image;
    std::string dockerfile;
    std::string address;
    std::string schedule_file;
    
    // RT configuration
    std::string rt_policy;
    int rt_priority;
    int rt_cpu_affinity;
    bool lock_memory;
    
    // Resources
    std::string cpuset;
    int64_t memlock;
    
    // Capabilities
    std::vector<std::string> capabilities;
    
    // Health check
    std::string healthcheck_command;
    int healthcheck_interval;
    int healthcheck_timeout;
    int healthcheck_retries;
};

// GitLab Registry configuration
struct GitLabRegistryConfig {
    std::string registry_url;
    std::string project_path;
    std::string username;
    std::string access_token;
    bool use_ci_token;
    bool enabled;
};

// Deployment configuration
struct DeploymentConfig {
    std::string name;
    std::string description;
    std::string network_name;
    std::string network_driver;
    
    OrchestratorConfig orchestrator;
    std::vector<ContainerConfig> tasks;
    
    // GitLab configuration
    GitLabRegistryConfig gitlab;
    
    // Deployment strategy
    std::string strategy_type;
    bool wait_for_healthy;
    int startup_timeout;
};

// Deploy Manager class
class DeployManager {
public:
    DeployManager();
    ~DeployManager();
    
    // Load deployment configuration from YAML
    bool load_config(const std::string& config_path);
    
    // GitLab authentication
    bool login_to_gitlab();
    bool logout_from_gitlab();
    
    // Pull Docker images from GitLab
    bool pull_all_images();
    bool pull_image(const std::string& image_name);
    std::string get_gitlab_image_name(const std::string& local_image);
    
    // Deploy containers
    bool deploy_all();
    bool deploy_tasks_only();
    bool deploy_orchestrator();
    bool deploy_task(const ContainerConfig& task_config);
    
    // Container management
    bool start_container(const std::string& container_name);
    bool stop_container(const std::string& container_name);
    bool remove_container(const std::string& container_name);
    
    // Network management
    bool create_network();
    bool remove_network();
    
    // Health checks
    bool wait_for_healthy(const std::string& container_name, int timeout_seconds);
    bool is_container_healthy(const std::string& container_name);
    
    // Cleanup
    bool cleanup_all();
    
    // Status
    void print_status();
    std::vector<std::string> get_running_containers();
    
    // Logs
    void follow_logs(const std::string& container_name = "");
    
    // Getters
    const DeploymentConfig& get_config() const { return config_; }
    
private:
    DeploymentConfig config_;
    bool config_loaded_;
    bool gitlab_logged_in_;
    
    // Helper methods
    std::string get_gitlab_token();
    std::string execute_command(const std::string& command);
    bool execute_command_bool(const std::string& command);
    std::string generate_docker_run_command(const ContainerConfig& config);
    std::string generate_orchestrator_run_command();
};

// Parser for deployment configuration
class DeploymentConfigParser {
public:
    static DeploymentConfig parse_yaml(const std::string& yaml_path);
};

} // namespace orchestrator
