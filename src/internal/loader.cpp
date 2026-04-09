#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

extern char **environ;

int main(int argc, char *argv[]) {
    // 1. Path to the original Roblox
    const char* original_app = "/Applications/Roblox.app";
    const char* original_binary = "/Applications/Roblox.app/Contents/MacOS/RobloxPlayer";
    
    // 2. Create a temporary "Safe" Roblox (Mirroring MacSploit)
    std::string temp_dir = "/tmp/rcl_temp";
    std::string safe_app = temp_dir + "/Roblox.app";
    std::string safe_binary = safe_app + "/Contents/MacOS/RobloxPlayer";
    
    // 3. Absolute path to our dylib
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) == NULL) return 1;
    std::string dylib_path = std::string(current_path) + "/bin/rcl_internal.dylib";
    
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] MacSploit-Style Binary Patching" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    // Create temp directory
    mkdir(temp_dir.c_str(), 0777);
    
    // Step 1: Copy Roblox App bundle (MacSploit method)
    if (access(safe_app.c_str(), F_OK) == -1) {
        std::cout << "[STEP 1] Mirroring Roblox to safe zone..." << std::endl;
        std::string copy_cmd = "cp -R \"" + std::string(original_app) + "\" \"" + safe_app + "\"";
        system(copy_cmd.c_str());
    }
    
    // Step 2: Use insert_dylib to patch the binary (The secret sauce)
    // This tells Roblox to load our dylib as soon as it starts
    std::cout << "[STEP 2] Patching binary with insert_dylib..." << std::endl;
    
    // First, remove signature
    system(("codesign --remove-signature \"" + safe_binary + "\" 2>/dev/null").c_str());
    
    // Use insert_dylib (We'll assume it's available in the project or installed)
    // --inplace: Patch the file directly
    // --all-yes: Confirm all prompts
    std::string patch_cmd = "./src/internal/insert_dylib --inplace --all-yes \"" + dylib_path + "\" \"" + safe_binary + "\" 2>&1";
    system(patch_cmd.c_str());
    
    // Step 3: Launch the patched Roblox
    std::cout << "[STEP 3] Launching patched Roblox..." << std::endl;
    pid_t pid;
    char* safe_binary_cstr = (char*)safe_binary.c_str();
    char* const args[] = {safe_binary_cstr, nullptr};
    
    int status = posix_spawn(&pid, safe_binary.c_str(), nullptr, nullptr, args, environ);
    
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
