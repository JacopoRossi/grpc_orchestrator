#include "builder_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <array>
#include <memory>

namespace orchestrator {

BuilderManager::BuilderManager() 
    : config_loaded_(false), gitlab_logged_in_(false) {
}

BuilderManager::~BuilderManager() {
    if (gitlab_logged_in_) {
        logout_from_gitlab();
    }
}

bool BuilderManager::load_config(const std::string& config_path) {
    std::cout << "[BuilderManager] Loading configuration from: " << config_path << std::endl;
    
    try {
        config_ = BuilderConfigParser::parse_yaml(config_path);
        config_loaded_ = true;
        
        std::cout << "[BuilderManager] Configuration loaded successfully" << std::endl;
        std::cout << "  Build configuration: " << config_.name << std::endl;
        std::cout << "  Images to build: " << config_.images.size() << std::endl;
        std::cout << "  GitLab registry: " << config_.gitlab.registry_url << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[BuilderManager] Failed to load configuration: " << e.what() << std::endl;
        return false;
    }
}

std::string BuilderManager::execute_command(const std::string& command) {
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

bool BuilderManager::execute_command_bool(const std::string& command) {
    int ret = system(command.c_str());
    return ret == 0;
}

std::string BuilderManager::get_gitlab_token() {
    // If using CI token, try to get it from environment
    if (config_.gitlab.use_ci_token) {
        const char* ci_token = std::getenv("CI_JOB_TOKEN");
        if (ci_token) {
            std::cout << "[BuilderManager] Using CI_JOB_TOKEN for authentication" << std::endl;
            return std::string(ci_token);
        } else {
            std::cout << "[BuilderManager] CI_JOB_TOKEN not found, falling back to access_token" << std::endl;
        }
    }
    
    // Otherwise use the configured access token
    return config_.gitlab.access_token;
}

bool BuilderManager::login_to_gitlab() {
    if (!config_loaded_) {
        std::cerr << "[BuilderManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Logging in to GitLab registry..." << std::endl;
    
    std::string token = get_gitlab_token();
    if (token.empty()) {
        std::cerr << "[BuilderManager] No GitLab token available" << std::endl;
        return false;
    }
    
    // Use docker login with token
    std::string command = "echo " + token + " | docker login " + 
                         config_.gitlab.registry_url + 
                         " -u " + config_.gitlab.username + 
                         " --password-stdin 2>&1";
    
    std::string result = execute_command(command);
    
    if (result.find("Login Succeeded") != std::string::npos) {
        std::cout << "[BuilderManager] Successfully logged in to GitLab registry" << std::endl;
        gitlab_logged_in_ = true;
        return true;
    } else {
        std::cerr << "[BuilderManager] Failed to login to GitLab registry" << std::endl;
        std::cerr << "  Output: " << result << std::endl;
        return false;
    }
}

bool BuilderManager::logout_from_gitlab() {
    std::cout << "[BuilderManager] Logging out from GitLab registry..." << std::endl;
    
    std::string command = "docker logout " + config_.gitlab.registry_url;
    bool success = execute_command_bool(command);
    
    if (success) {
        gitlab_logged_in_ = false;
        std::cout << "[BuilderManager] Successfully logged out" << std::endl;
    }
    
    return success;
}

bool BuilderManager::build_image(const ImageBuildConfig& image_config) {
    std::cout << "[BuilderManager] Building image: " << image_config.image_name 
              << ":" << image_config.image_tag << std::endl;
    
    std::stringstream cmd;
    cmd << "docker build";
    
    if (image_config.no_cache) {
        cmd << " --no-cache";
    }
    
    cmd << " -f " << image_config.dockerfile;
    cmd << " -t " << image_config.image_name << ":" << image_config.image_tag;
    
    // Add build arguments
    for (const auto& [key, value] : image_config.build_args) {
        cmd << " --build-arg " << key << "=" << value;
    }
    
    cmd << " " << image_config.build_context;
    
    std::cout << "[BuilderManager] Executing: " << cmd.str() << std::endl;
    
    if (!execute_command_bool(cmd.str())) {
        std::cerr << "[BuilderManager] Failed to build image: " 
                  << image_config.image_name << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Image built successfully" << std::endl;
    return true;
}

bool BuilderManager::build_all_images() {
    if (!config_loaded_) {
        std::cerr << "[BuilderManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Building all images..." << std::endl;
    
    for (const auto& image_config : config_.images) {
        if (!build_image(image_config)) {
            std::cerr << "[BuilderManager] Build failed for: " 
                      << image_config.image_name << std::endl;
            return false;
        }
    }
    
    std::cout << "[BuilderManager] All images built successfully" << std::endl;
    return true;
}

std::string BuilderManager::get_gitlab_image_name(const std::string& image_name, 
                                                   const std::string& tag) {
    return config_.gitlab.registry_url + "/" + 
           config_.gitlab.project_path + "/" + 
           image_name + ":" + tag;
}

bool BuilderManager::tag_for_gitlab(const std::string& local_image, 
                                     const std::string& local_tag) {
    std::string gitlab_image = get_gitlab_image_name(local_image, local_tag);
    std::string local_full = local_image + ":" + local_tag;
    
    std::cout << "[BuilderManager] Tagging image for GitLab:" << std::endl;
    std::cout << "  Local:  " << local_full << std::endl;
    std::cout << "  GitLab: " << gitlab_image << std::endl;
    
    std::string command = "docker tag " + local_full + " " + gitlab_image;
    
    if (!execute_command_bool(command)) {
        std::cerr << "[BuilderManager] Failed to tag image" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Image tagged successfully" << std::endl;
    return true;
}

bool BuilderManager::push_to_gitlab(const std::string& image_name, 
                                     const std::string& tag) {
    if (!gitlab_logged_in_) {
        std::cerr << "[BuilderManager] Not logged in to GitLab registry" << std::endl;
        return false;
    }
    
    std::string gitlab_image = get_gitlab_image_name(image_name, tag);
    
    std::cout << "[BuilderManager] Pushing image to GitLab: " << gitlab_image << std::endl;
    
    std::string command = "docker push " + gitlab_image;
    
    if (!execute_command_bool(command)) {
        std::cerr << "[BuilderManager] Failed to push image" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Image pushed successfully" << std::endl;
    
    // Cleanup local GitLab-tagged image if requested
    if (config_.cleanup_local_after_push) {
        std::cout << "[BuilderManager] Cleaning up local GitLab-tagged image..." << std::endl;
        std::string cleanup_cmd = "docker rmi " + gitlab_image + " 2>/dev/null || true";
        execute_command_bool(cleanup_cmd);
    }
    
    return true;
}

bool BuilderManager::push_all_to_gitlab() {
    if (!config_loaded_) {
        std::cerr << "[BuilderManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    if (!gitlab_logged_in_) {
        std::cout << "[BuilderManager] Not logged in, attempting login..." << std::endl;
        if (!login_to_gitlab()) {
            return false;
        }
    }
    
    std::cout << "[BuilderManager] Pushing all images to GitLab..." << std::endl;
    
    for (const auto& image_config : config_.images) {
        // Tag for GitLab
        if (!tag_for_gitlab(image_config.image_name, image_config.image_tag)) {
            std::cerr << "[BuilderManager] Failed to tag: " 
                      << image_config.image_name << std::endl;
            return false;
        }
        
        // Push to GitLab
        if (!push_to_gitlab(image_config.image_name, image_config.image_tag)) {
            std::cerr << "[BuilderManager] Failed to push: " 
                      << image_config.image_name << std::endl;
            return false;
        }
    }
    
    std::cout << "[BuilderManager] All images pushed successfully" << std::endl;
    return true;
}

bool BuilderManager::build_and_push_all() {
    if (!config_loaded_) {
        std::cerr << "[BuilderManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Starting build and push workflow..." << std::endl;
    
    // Build all images
    if (!build_all_images()) {
        std::cerr << "[BuilderManager] Build phase failed" << std::endl;
        return false;
    }
    
    // Push if configured
    if (config_.push_after_build) {
        if (!login_to_gitlab()) {
            std::cerr << "[BuilderManager] Login failed, skipping push" << std::endl;
            return false;
        }
        
        if (!push_all_to_gitlab()) {
            std::cerr << "[BuilderManager] Push phase failed" << std::endl;
            return false;
        }
    }
    
    std::cout << "[BuilderManager] Build and push workflow completed successfully" << std::endl;
    return true;
}

bool BuilderManager::remove_local_image(const std::string& image_name, 
                                         const std::string& tag) {
    std::string full_name = image_name + ":" + tag;
    std::cout << "[BuilderManager] Removing local image: " << full_name << std::endl;
    
    std::string command = "docker rmi " + full_name + " 2>/dev/null || true";
    return execute_command_bool(command);
}

bool BuilderManager::cleanup_local_images() {
    if (!config_loaded_) {
        std::cerr << "[BuilderManager] Configuration not loaded" << std::endl;
        return false;
    }
    
    std::cout << "[BuilderManager] Cleaning up local images..." << std::endl;
    
    for (const auto& image_config : config_.images) {
        remove_local_image(image_config.image_name, image_config.image_tag);
        
        // Also remove GitLab-tagged version
        std::string gitlab_image = get_gitlab_image_name(image_config.image_name, 
                                                         image_config.image_tag);
        std::string cmd = "docker rmi " + gitlab_image + " 2>/dev/null || true";
        execute_command_bool(cmd);
    }
    
    std::cout << "[BuilderManager] Cleanup completed" << std::endl;
    return true;
}

void BuilderManager::print_status() {
    std::cout << "\n=== Builder Status ===" << std::endl;
    std::cout << "Configuration: " << config_.name << std::endl;
    std::cout << "GitLab Registry: " << config_.gitlab.registry_url << std::endl;
    std::cout << "Project Path: " << config_.gitlab.project_path << std::endl;
    std::cout << "Logged in: " << (gitlab_logged_in_ ? "Yes" : "No") << std::endl;
    
    std::cout << "\nImages to build:" << std::endl;
    for (const auto& img : config_.images) {
        std::cout << "  - " << img.image_name << ":" << img.image_tag << std::endl;
    }
    
    auto local_images = get_local_images();
    std::cout << "\nLocal images: " << local_images.size() << std::endl;
    for (const auto& image : local_images) {
        std::cout << "  - " << image << std::endl;
    }
}

std::vector<std::string> BuilderManager::get_local_images() {
    std::vector<std::string> images;
    
    for (const auto& img : config_.images) {
        std::string full_name = img.image_name + ":" + img.image_tag;
        std::string command = "docker images --format '{{.Repository}}:{{.Tag}}' | grep '^" + 
                             full_name + "$'";
        std::string result = execute_command(command);
        
        if (!result.empty()) {
            // Remove trailing newline
            if (result.back() == '\n') {
                result.pop_back();
            }
            images.push_back(result);
        }
    }
    
    return images;
}

// Helper functions for YAML parsing
namespace {
    template<typename T>
    T get_yaml(const YAML::Node& node, const std::string& key, T default_val = T()) {
        return node[key] ? node[key].as<T>() : default_val;
    }
}

// Parser implementation
BuilderConfig BuilderConfigParser::parse_yaml(const std::string& yaml_path) {
    YAML::Node config = YAML::LoadFile(yaml_path);
    BuilderConfig builder;
    
    YAML::Node build_node = config["builder"];
    
    // Basic info
    builder.name = build_node["name"].as<std::string>();
    builder.description = build_node["description"].as<std::string>();
    
    // GitLab configuration
    YAML::Node gitlab = build_node["gitlab"];
    builder.gitlab.registry_url = gitlab["registry_url"].as<std::string>();
    builder.gitlab.project_path = gitlab["project_path"].as<std::string>();
    builder.gitlab.username = get_yaml<std::string>(gitlab, "username", "gitlab-ci-token");
    builder.gitlab.access_token = get_yaml<std::string>(gitlab, "access_token", "");
    builder.gitlab.use_ci_token = get_yaml<bool>(gitlab, "use_ci_token", false);
    
    // Images
    for (const auto& img_node : build_node["images"]) {
        ImageBuildConfig image;
        image.image_name = img_node["name"].as<std::string>();
        image.image_tag = get_yaml<std::string>(img_node, "tag", "latest");
        image.dockerfile = img_node["dockerfile"].as<std::string>();
        image.build_context = get_yaml<std::string>(img_node, "context", ".");
        image.no_cache = get_yaml<bool>(img_node, "no_cache", false);
        
        // Build args
        if (img_node["build_args"]) {
            for (const auto& arg : img_node["build_args"]) {
                std::string key = arg.first.as<std::string>();
                std::string value = arg.second.as<std::string>();
                image.build_args[key] = value;
            }
        }
        
        builder.images.push_back(image);
    }
    
    // Build settings
    YAML::Node settings = build_node["settings"];
    builder.build_parallel = get_yaml<bool>(settings, "parallel", false);
    builder.push_after_build = get_yaml<bool>(settings, "push_after_build", true);
    builder.cleanup_local_after_push = get_yaml<bool>(settings, "cleanup_after_push", false);
    
    return builder;
}

} // namespace orchestrator
