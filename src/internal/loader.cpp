#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

extern char **environ;

int main(int argc, char *argv[]) {
    // Path to the Roblox executable
    const char* original_roblox = "/Applications/Roblox.app/Contents/MacOS/RobloxPlayer";
    
    // Create a temporary path for our "Safe" Roblox
    std::string temp_dir = "/tmp/rcl_temp";
    std::string safe_roblox = temp_dir + "/RobloxPlayer";
    
    // Path to our compiled dylib (Must be absolute)
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        std::cerr << "[ERROR] Could not get current directory" << std::endl;
        return 1;
    }
    std::string dylib_path = std::string(current_path) + "/bin/rcl_internal.dylib";
    
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] Preparing SIP-Friendly Injection..." << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    // 1. Create temp directory
    mkdir(temp_dir.c_str(), 0777);
    
    // 2. Copy Roblox to temp
    std::cout << "[STEP 1] Copying Roblox to safe zone..." << std::endl;
    std::string copy_cmd = "cp \"" + std::string(original_roblox) + "\" \"" + safe_roblox + "\"";
    system(copy_cmd.c_str());
    
    // 3. Strip Code Signature (Bypasses Library Validation)
    std::cout << "[STEP 2] Stripping signatures (Bypassing SIP)..." << std::endl;
    std::string strip_cmd = "codesign --remove-signature \"" + safe_roblox + "\" 2>/dev/null";
    system(strip_cmd.c_str());
    
    // 4. Set Environment
    std::string env_var = "DYLD_INSERT_LIBRARIES=" + dylib_path;
    
    std::vector<char*> new_env;
    new_env.push_back((char*)env_var.c_str());
    for (char **env = environ; *env != nullptr; ++env) {
        new_env.push_back(*env);
    }
    new_env.push_back(nullptr);
    
    // 5. Launch
    std::cout << "[STEP 3] Launching Safe Roblox..." << std::endl;
    pid_t pid;
    char* safe_roblox_cstr = (char*)safe_roblox.c_str();
    char* const args[] = {safe_roblox_cstr, nullptr};
    
    int status = posix_spawn(&pid, safe_roblox.c_str(), nullptr, nullptr, args, new_env.data());
    
    if (status == 0) {
        std::cout << "[SUCCESS] Internal Injection Active! (PID: " << pid << ")" << std::endl;
        std::cout << "[NOTE] You did NOT have to disable SIP. Happy Scripting!" << std::endl;
        
        int wait_status;
        waitpid(pid, &wait_status, 0);
        std::cout << "[INFO] Roblox closed." << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to launch: " << status << std::endl;
    }
    
    return 0;
}
