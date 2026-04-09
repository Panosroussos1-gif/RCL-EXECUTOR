#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

extern char **environ;

int main(int argc, char *argv[]) {
    // List of possible Roblox installation paths
    std::vector<std::string> possible_apps = {
        "/Applications/Roblox.app",
        "/Users/" + std::string(getlogin() ? getlogin() : "User") + "/Applications/Roblox.app",
        "/Applications/RobloxPlayer.app"
    };
    
    std::string original_app = "";
    for (const auto& path : possible_apps) {
        if (access(path.c_str(), F_OK) != -1) {
            original_app = path;
            break;
        }
    }

    if (original_app == "") {
        std::cerr << "[ERROR] Roblox application not found in standard locations." << std::endl;
        return 1;
    }
    
    // Create a temporary path for our "Safe" Roblox
    std::string temp_dir = "/tmp/rcl_temp";
    std::string safe_app = temp_dir + "/Roblox.app";
    
    // Try to find the binary inside the bundle (RobloxPlayer or RobloxPlayerBeta)
    std::string safe_binary = "";
    std::vector<std::string> binary_names = {"RobloxPlayer", "RobloxPlayerBeta", "Roblox"};
    
    // Path to our compiled dylib (Must be absolute)
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        std::cerr << "[ERROR] Could not get current directory" << std::endl;
        return 1;
    }
    std::string dylib_path = std::string(current_path) + "/bin/rcl_internal.dylib";
    
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] Preparing SIP-Friendly Injection..." << std::endl;
    std::cout << "[INFO] Found Roblox at: " << original_app << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    // 1. Create temp directory
    mkdir(temp_dir.c_str(), 0777);
    
    // 2. Copy the ENTIRE Roblox.app bundle to safe zone
    if (access(safe_app.c_str(), F_OK) == -1) {
        std::cout << "[STEP 1] Copying Roblox App bundle to safe zone..." << std::endl;
        std::string copy_cmd = "cp -R \"" + original_app + "\" \"" + safe_app + "\"";
        system(copy_cmd.c_str());
    }
    
    // Find the actual binary in the copied bundle
    for (const auto& name : binary_names) {
        std::string test_path = safe_app + "/Contents/MacOS/" + name;
        if (access(test_path.c_str(), F_OK) != -1) {
            safe_binary = test_path;
            break;
        }
    }

    if (safe_binary == "") {
        std::cerr << "[ERROR] Could not find Roblox binary inside the app bundle." << std::endl;
        return 1;
    }
    
    // 3. Strip Code Signature (Bypasses Library Validation)
    std::cout << "[STEP 2] Stripping signatures (Bypassing SIP)..." << std::endl;
    std::string strip_cmd = "codesign --remove-signature \"" + safe_binary + "\" 2>/dev/null";
    system(strip_cmd.c_str());
    
    // 4. Set Environment
    std::string env_var = "DYLD_INSERT_LIBRARIES=" + dylib_path;
    
    std::vector<char*> new_env;
    new_env.push_back((char*)env_var.c_str());
    for (char **env = environ; *env != nullptr; ++env) {
        new_env.push_back(*env);
    }
    new_env.push_back(nullptr);
    
    // 5. Launch the Safe App
    std::cout << "[STEP 3] Launching Safe Roblox..." << std::endl;
    pid_t pid;
    char* safe_binary_cstr = (char*)safe_binary.c_str();
    char* const args[] = {safe_binary_cstr, nullptr};
    
    int status = posix_spawn(&pid, safe_binary.c_str(), nullptr, nullptr, args, new_env.data());
    
    if (status == 0) {
        std::cout << "[SUCCESS] Internal Injection Active! (PID: " << pid << ")" << std::endl;
        
        int wait_status;
        waitpid(pid, &wait_status, 0);
        std::cout << "[INFO] Roblox closed." << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to launch: " << status << std::endl;
    }
    
    return 0;
}
