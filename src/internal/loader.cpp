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
    // Look for processes that are actually running and not just launchers
    std::string cmd = "ps -A | grep -i " + name + " | grep -v grep | awk '{print $1}'";
    FILE* pipe = popen(cmd.c_str(), "r");
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
    // We search for "RobloxPlayer" which is the actual engine process
    pid_t pid = find_pid_by_name("RobloxPlayer");
    
    if (pid == 0) {
        // Fallback to "Roblox" if the name is different
        pid = find_pid_by_name("Roblox");
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
    
    if (access(dylib_path.c_str(), F_OK) == -1) {
        std::cerr << "ERROR: Internal Engine (dylib) not found at: " << dylib_path << std::endl;
        return 1;
    }

    // 3. Injection via lldb
    std::cout << "[STEP 2] Injecting dylib into memory..." << std::endl;
    
    // Simplified and more direct lldb command
    // We use --attach-pid directly and run the expression without manual interrupt
    std::string lldb_cmd = "lldb --attach-pid " + std::to_string(pid) + " --batch " +
                          "-o 'expression (void*)dlopen(\"" + dylib_path + "\", 10)' " +
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
        
        // Check for success markers
        if (line.find("0x") != std::string::npos && line.find("$") != std::string::npos) {
            success = true; 
        }
        if (line.find("error:") != std::string::npos) {
            // Some errors are non-fatal, but we log them
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
        std::cerr << "[ERROR] Injection failed. Roblox might be blocking the debugger." << std::endl;
        std::cerr << "FIX: Run 'sudo DevToolsSecurity --enable' in your terminal." << std::endl;
        std::cerr << "------------------------------------------" << std::endl;
        return 1;
    }

    return 0;
}
