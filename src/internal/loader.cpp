#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

extern char **environ;

// Function to find the PID of a running process by name
pid_t find_pid_by_name(const std::string& name) {
    FILE* pipe = popen(("pgrep -x " + name).c_str(), "r");
    if (!pipe) return 0;
    char buffer[128];
    pid_t pid = 0;
    if (fgets(buffer, 128, pipe) != NULL) {
        try {
            pid = std::stoi(buffer);
        } catch (...) {
            pid = 0;
        }
    }
    pclose(pipe);
    return pid;
}

int main(int argc, char *argv[]) {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] MacSploit-Style Injection (Live)" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // 1. Find the LIVE Roblox process
    std::string process_name = "RobloxPlayer";
    pid_t pid = find_pid_by_name(process_name);
    
    if (pid == 0) {
        process_name = "RobloxPlayerBeta";
        pid = find_pid_by_name(process_name);
    }

    if (pid == 0) {
        std::cerr << "[ERROR] Roblox is not running! Please open Roblox first." << std::endl;
        return 1;
    }

    std::cout << "[STEP 1] Found live Roblox process (PID: " << pid << ")" << std::endl;

    // 2. Get the absolute path to our dylib
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) == NULL) return 1;
    std::string dylib_path = std::string(current_path) + "/bin/rcl_internal.dylib";

    // 3. Injection via lldb (The MacSploit method for live processes)
    // This script tells lldb to attach to the PID and load our dylib
    std::cout << "[STEP 2] Injecting dylib into memory..." << std::endl;
    
    std::string lldb_cmd = "lldb -p " + std::to_string(pid) + " --batch " +
                          "-o 'process interrupt' " +
                          "-o 'expression (void*)dlopen(\"" + dylib_path + "\", 10)' " +
                          "-o 'process continue' " +
                          "-o 'detach'";
    
    int result = system(lldb_cmd.c_str());

    if (result == 0) {
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "[SUCCESS] RCL Internal injected into live Roblox!" << std::endl;
        std::cout << "[INFO] You can now use the executor." << std::endl;
        std::cout << "------------------------------------------" << std::endl;
    } else {
        std::cerr << "[ERROR] Injection failed. You may need to grant Terminal 'Developer Tools' access in System Settings." << std::endl;
    }

    return 0;
}
