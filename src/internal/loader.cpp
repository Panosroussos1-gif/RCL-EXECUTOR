#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>

extern char **environ;

int main(int argc, char *argv[]) {
    // Path to the Roblox executable
    const char* roblox_path = "/Applications/Roblox.app/Contents/MacOS/RobloxPlayer";
    
    // Path to our compiled dylib
    const char* dylib_path = "./bin/rcl_internal.dylib";
    
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] Launching Roblox with RCL Internal..." << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    // Set the environment variable for injection
    // DYLD_INSERT_LIBRARIES=path/to/dylib
    std::string env_var = "DYLD_INSERT_LIBRARIES=";
    env_var += dylib_path;
    
    // Build the environment array
    std::vector<char*> new_env;
    new_env.push_back((char*)env_var.c_str());
    
    // Keep existing environment
    for (char **env = environ; *env != nullptr; ++env) {
        new_env.push_back(*env);
    }
    new_env.push_back(nullptr);
    
    // Launch the process
    pid_t pid;
    const char* args[] = {roblox_path, nullptr};
    
    int status = posix_spawn(&pid, roblox_path, nullptr, nullptr, (char* const*)args, new_env.data());
    
    if (status == 0) {
        std::cout << "[SUCCESS] Roblox launched (PID: " << pid << ")" << std::endl;
        std::cout << "[STATUS] Injection initiated. Check system logs for output." << std::endl;
        
        // Wait for the process to exit
        int wait_status;
        waitpid(pid, &wait_status, 0);
        std::cout << "[INFO] Roblox closed." << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to launch Roblox: " << status << std::endl;
        std::cerr << "[HINT] Make sure Roblox is installed in /Applications and SIP is disabled." << std::endl;
    }
    
    return 0;
}
