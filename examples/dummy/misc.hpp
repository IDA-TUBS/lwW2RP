#ifndef MISC_h
#define MISC_h

#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

#include <w2rp/log.hpp>

std::string setup_defines = "examples/cfg/setup_defines.json";
std::string w2rp_config = "examples/cfg/w2rp_config.json";

/** 
 * @brief Get the path of the currently running executable
*/
std::filesystem::path get_executable_path() 
{
    // Linux: Read /proc/self/exe
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0) {
        return std::filesystem::path(result, result + count);
    }
    return {}; // Return an empty path on failure
}

/** 
 * @brief Find the repository root by looking for a marker file/directory
*/
std::filesystem::path find_repository_root(const std::filesystem::path& exec_path) 
{
    std::filesystem::path current_dir = exec_path.parent_path();
    const int MAX_UP_LEVELS = 10; // Prevent infinite loop

    for (int i = 0; i < MAX_UP_LEVELS; ++i) {
        // Look for a unique marker file/directory (e.g., the .git folder)
        if (std::filesystem::exists(current_dir / ".git")) {
            return current_dir; // Found the root!
        }
        
        // Move up one directory
        std::filesystem::path parent_dir = current_dir.parent_path();
        if (parent_dir == current_dir) {
             // Reached the filesystem root (e.g., '/') and didn't find the marker
            break;
        }
        current_dir = parent_dir;
    }

    std::cerr << "ERROR: Could not find repository root. Marker not found." << std::endl;
    return {};
}

/** 
 * @brief Resolve the absolute path to a configuration file given its relative path from the repository root
*/
std::string resolve_config_path(const std::string& relative_config_path) 
{
    std::filesystem::path executable_path = get_executable_path();
    if (executable_path.empty()) {
        std::cerr << "Failed to determine executable path." << std::endl;
        return{};
    }

    std::filesystem::path repo_root = find_repository_root(executable_path);
    if (repo_root.empty()) {
        return{}; // Error message printed in find_repository_root
    }

    std::filesystem::path config_path = repo_root / relative_config_path;

    return config_path.string();
}

std::string get_log_dir()
{
    std::filesystem::path executable_path = get_executable_path();
    if (executable_path.empty()) {
        std::cerr << "Failed to determine executable path." << std::endl;
        return{};
    }

    std::filesystem::path repo_root = find_repository_root(executable_path);
    if (repo_root.empty()) {
        return{}; // Error message printed in find_repository_root
    }

    std::filesystem::path log_dir = repo_root / "examples/logs/";

    return log_dir.string();
}

/**
 * @brief generate file name suffix based on current ime
 * 
 * @return std::string 
 */
std::string generateTimestamp() {
    // Get the current time
    std::time_t now = std::time(nullptr);
    
    // Convert to a tm structure
    std::tm* localTime = std::localtime(&now);
    
    // Create a string stream to format the date and time
    std::stringstream ss;
    ss << std::put_time(localTime, "%Y%m%d_%H%M%S");

    // Return the formatted string
    return ss.str();
}

# endif