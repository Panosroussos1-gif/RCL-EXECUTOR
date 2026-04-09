#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

// Simple function to make an HTTP GET request in C++
std::string http_get(const std::string& host, int port, const std::string& path) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return "";

    struct hostent* server = gethostbyname(host.c_str());
    if (server == NULL) return "";

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return "";
    }

    std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    send(sockfd, request.c_str(), request.length(), 0);

    char buffer[4096];
    std::string response;
    int n;
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        response += buffer;
    }
    close(sockfd);

    size_t pos = response.find("\r\n\r\n");
    if (pos != std::string::npos) {
        return response.substr(pos + 4);
    }
    return "";
}

// Function that runs as soon as the dylib is loaded
__attribute__((constructor))
static void initialize() {
    std::thread([]() {
        sleep(2);
        
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "[RCL] Internal Engine Connected (M-Series)" << std::endl;
        std::cout << "[RCL] Polling for scripts..." << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        
        while (true) {
            // Send Heartbeat to let the app know we are alive
            http_get("127.0.0.1", 5500, "/heartbeat");
            
            // Poll the Electron server for new scripts
            std::string script = http_get("127.0.0.1", 5500, "/get-script");
            
            if (!script.empty()) {
                std::cout << "[RCL] Executing script (Internal): " << script.substr(0, 50) << "..." << std::endl;
            }
            
            usleep(500000); // Poll every 500ms
        }
    }).detach();
}

// Exported function that Electron can call if we use a bridge
extern "C" void ExecuteScript(const char* source) {
    std::cout << "[RCL] Executing Internal Script: " << source << std::endl;
}
