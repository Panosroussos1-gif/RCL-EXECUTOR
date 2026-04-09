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
        std::cerr << "ERROR: Roblox is not running! Open the game first." << std::endl;
        return 1;
    }

    std::cout << "[STEP 1] Found live Roblox process (PID: " << pid << ")" << std::endl;

    // 2. Get the absolute path to our dylib
    char current_path[1024];
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        std::cerr << "ERROR: Could not get current directory" << std::endl;
        return 1;
    }
    std::string dylib_path = std::string(current_path) + "/bin/rcl_internal.dylib";
    
    // Check if dylib exists
    if (access(dylib_path.c_str(), F_OK) == -1) {
        std::cerr << "ERROR: Internal Engine (dylib) not found at: " << dylib_path << std::endl;
        return 1;
    }

    // 3. Injection via lldb
    std::cout << "[STEP 2] Injecting dylib into memory..." << std::endl;
    
    // Use a more verbose lldb command to see what's happening
    std::string lldb_cmd = "lldb -p " + std::to_string(pid) + " --batch " +
                          "-o 'process interrupt' " +
                          "-o 'expression (void*)dlopen(\"" + dylib_path + "\", 10)' " +
                          "-o 'process continue' " +
                          "-o 'detach' 2>&1";
    
    FILE* lldb_pipe = popen(lldb_cmd.c_str(), "r");
    if (!lldb_pipe) {
        std::cerr << "ERROR: Failed to run lldb" << std::endl;
        return 1;
    }

    char lldb_buffer[512];
    bool success = false;
    while (fgets(lldb_buffer, 512, lldb_pipe) != NULL) {
        std::string line(lldb_buffer);
        std::cout << "  [LLDB] " << line;
        if (line.find("0x") != std::string::npos && line.find("$") != std::string::npos) {
            success = true; // dlopen returned a handle
        }
        if (line.find("error:") != std::string::npos) {
            std::cerr << "  [LLDB ERROR] " << line;
        }
    }
    pclose(lldb_pipe);

    if (success) {
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "[SUCCESS] RCL Internal injected successfully!" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
    } else {
        std::cerr << "------------------------------------------" << std::endl;
        std::cerr << "[ERROR] Injection failed. Check the LLDB logs above." << std::endl;
        std::cerr << "HINT: You may need to run 'sudo DevToolsSecurity --enable' in your terminal." << std::endl;
        std::cerr << "------------------------------------------" << std::endl;
        return 1;
    }

    return 0;
}
