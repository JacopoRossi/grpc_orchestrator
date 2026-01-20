#pragma once

#include <string>
#include <vector>
#include <map>

namespace orchestrator {

// GitLab Registry configuration
struct GitLabConfig {
    std::string registry_url;         // e.g., "registry.gitlab.com"
    std::string project_path;         // e.g., "username/project"
    std::string username;
    std::string access_token;         // GitLab Personal Access Token or CI token
    bool use_ci_token;                // Use CI_JOB_TOKEN if running in CI/CD
};

// Image build configuration
struct ImageBuildConfig {
    std::string dockerfile;           // Path to Dockerfile
    std::string image_name;           // Local image name
    std::string image_tag;            // Image tag (e.g., "latest", "v1.0.0")
    std::string build_context;        // Build context path
    bool no_cache;                    // Build without cache
    std::map<std::string, std::string> build_args;  // Build arguments
};

// Builder configuration
struct BuilderConfig {
    std::string name;
    std::string description;
    GitLabConfig gitlab;
    std::vector<ImageBuildConfig> images;
    
    // Build settings
    bool build_parallel;
    bool push_after_build;
    bool cleanup_local_after_push;
};

// Builder Manager class
class BuilderManager {
public:
    BuilderManager();
    ~BuilderManager();
    
    // Load builder configuration from YAML
    bool load_config(const std::string& config_path);
    
    // GitLab authentication
    bool login_to_gitlab();
    bool logout_from_gitlab();
    
    // Build Docker images
    bool build_all_images();
    bool build_image(const ImageBuildConfig& image_config);
    
    // Tag images for GitLab registry
    bool tag_for_gitlab(const std::string& local_image, const std::string& local_tag);
    std::string get_gitlab_image_name(const std::string& image_name, const std::string& tag);
    
    // Push images to GitLab
    bool push_all_to_gitlab();
    bool push_to_gitlab(const std::string& image_name, const std::string& tag);
    
    // Complete workflow: build + push
    bool build_and_push_all();
    
    // Cleanup
    bool cleanup_local_images();
    bool remove_local_image(const std::string& image_name, const std::string& tag);
    
    // Status
    void print_status();
    std::vector<std::string> get_local_images();
    
    // Getters
    const BuilderConfig& get_config() const { return config_; }
    
private:
    BuilderConfig config_;
    bool config_loaded_;
    bool gitlab_logged_in_;
    
    // Helper methods
    std::string execute_command(const std::string& command);
    bool execute_command_bool(const std::string& command);
    std::string get_gitlab_token();
};

// Parser for builder configuration
class BuilderConfigParser {
public:
    static BuilderConfig parse_yaml(const std::string& yaml_path);
};

} // namespace orchestrator
