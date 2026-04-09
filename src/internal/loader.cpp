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
    // 1. Setup Paths
    std::string home = getenv("HOME") ? getenv("HOME") : "";
    std::string safe_dir = home + "/Documents/.rcl_cache";
    std::string safe_app = safe_dir + "/Roblox.app";
    std::string safe_binary = safe_app + "/Contents/MacOS/RobloxPlayer";
    std::string internal_dylib_name = "rcl_internal.dylib";
    std::string safe_dylib = safe_app + "/Contents/MacOS/" + internal_dylib_name;
    
    const char* original_app = "/Applications/Roblox.app";

    // Find our own directory to locate insert_dylib and the engine
    char buf[1024];
    uint32_t size = sizeof(buf);
    std::string base_dir = "";
    if (getcwd(buf, size) != NULL) {
        base_dir = std::string(buf);
    }
    // Check if we are in bin/
    if (base_dir.find("/bin") != std::string::npos) {
        base_dir = base_dir.substr(0, base_dir.find("/bin"));
    }
    
    std::string engine_source = base_dir + "/bin/" + internal_dylib_name;
    std::string insert_dylib_tool = base_dir + "/src/internal/insert_dylib";

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[LOADER] MacSploit-Style Injection v2" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // Step 1: Create safe directory in Documents (more stable than /tmp)
    mkdir(safe_dir.c_str(), 0777);

    // Step 2: Mirror Roblox
    if (access(safe_app.c_str(), F_OK) == -1) {
        std::cout << "[STEP 1] Mirroring Roblox to safe zone..." << std::endl;
        system(("cp -R \"" + std::string(original_app) + "\" \"" + safe_app + "\"").c_str());
    }

    // Step 3: Copy Engine into the App Bundle (MacSploit Secret)
    std::cout << "[STEP 2] Bundling engine with Roblox..." << std::endl;
    system(("cp \"" + engine_source + "\" \"" + safe_dylib + "\"").c_str());
    system(("codesign --remove-signature \"" + safe_dylib + "\" 2>/dev/null").c_str());
    system(("codesign --remove-signature \"" + safe_binary + "\" 2>/dev/null").c_str());

    // Step 4: Patch binary using @executable_path
    // This tells Roblox: "Look for the dylib in the same folder as you"
    std::cout << "[STEP 3] Patching binary..." << std::endl;
    std::string patch_cmd = "\"" + insert_dylib_tool + "\" --inplace --all-yes \"@executable_path/" + internal_dylib_name + "\" \"" + safe_binary + "\" > /dev/null 2>&1";
    system(patch_cmd.c_str());

    // Step 5: Launch using the 'open' command (handles app bundles correctly)
    std::cout << "[STEP 4] Launching Roblox..." << std::endl;
    std::string launch_cmd = "open \"" + safe_app + "\"";
    int res = system(launch_cmd.c_str());

    if (res == 0) {
        std::cout << "[SUCCESS] Internal Injection Active!" << std::endl;
        std::cout << "[INFO] Roblox is opening. Enjoy!" << std::endl;
        return 0;
    } else {
        std::cerr << "[ERROR] Failed to launch Roblox bundle." << std::endl;
        return 1;
    }
}

