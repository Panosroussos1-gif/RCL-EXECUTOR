#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

extern char **environ;

// Function to get the absolute path of the current executable
std::string get_self_path() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (getcwd(path, size) != NULL) {
        return std::string(path);
    }
    return "";
}

int main(int argc, char *argv[]) {
    // 1. Find our own directory to locate insert_dylib
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1); // Linux
    if (len == -1) {
        // Mac fallback: Use argv[0]
        std::string arg0 = argv[0];
        char* dname = dirname((char*)arg0.c_str());
        strcpy(buf, dname);
    } else {
        buf[len] = '\0';
        char* dname = dirname(buf);
        strcpy(buf, dname);
    }
    std::string base_dir = std::string(buf);
    
    // Path to insert_dylib (Should be in the same folder as this loader)
    std::string insert_dylib_tool = base_dir + "/insert_dylib";
    
    // Fallback for Dev mode
    if (access(insert_dylib_tool.c_str(), F_OK) == -1) {
        insert_dylib_tool = "./src/internal/insert_dylib";
    }

    // 2. Path to the original Roblox
    const char* original_app = "/Applications/Roblox.app";
    const char* original_binary = "/Applications/Roblox.app/Contents/MacOS/RobloxPlayer";
    
    // 3. Create a temporary "Safe" Roblox (Mirroring MacSploit)
    std::string temp_dir = "/tmp/rcl_temp";
    std::string safe_app = temp_dir + "/Roblox.app";
    std::string safe_binary = safe_app + "/Contents/MacOS/RobloxPlayer";
    
    // 4. Absolute path to our dylib (In the same folder as loader)
    std::string dylib_path = base_dir + "/rcl_internal.dylib";
    if (access(dylib_path.c_str(), F_OK) == -1) {
        dylib_path = "./bin/rcl_internal.dylib";
    }
    
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] MacSploit-Style Injection" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    mkdir(temp_dir.c_str(), 0777);
    
    if (access(safe_app.c_str(), F_OK) == -1) {
        std::cout << "[STEP 1] Mirroring Roblox..." << std::endl;
        system(("cp -R \"" + std::string(original_app) + "\" \"" + safe_app + "\"").c_str());
    }
    
    std::cout << "[STEP 2] Patching binary..." << std::endl;
    system(("codesign --remove-signature \"" + safe_binary + "\" 2>/dev/null").c_str());
    
    std::string patch_cmd = "\"" + insert_dylib_tool + "\" --inplace --all-yes \"" + dylib_path + "\" \"" + safe_binary + "\" 2>&1";
    system(patch_cmd.c_str());
    
    std::cout << "[STEP 3] Launching patched Roblox..." << std::endl;
    pid_t pid;
    char* safe_binary_cstr = (char*)safe_binary.c_str();
    char* const args[] = {safe_binary_cstr, nullptr};
    
    int status = posix_spawn(&pid, safe_binary.c_str(), nullptr, nullptr, args, environ);
    
    if (status == 0) {
        std::cout << "[SUCCESS] Internal Injection Active!" << std::endl;
        int wait_status;
        waitpid(pid, &wait_status, 0);
    } else {
        std::cerr << "[ERROR] Failed to launch Roblox." << std::endl;
    }
    
    return 0;
}

