#include "system_monitor.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ─── Simple HTTP POST (no external deps) ─────────────────────────────────────
static bool httpPost(const std::string& host, int port,
                     const std::string& path, const std::string& body) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(),
                         &hints, &res);
    if (rc != 0 || !res) return false;

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return false; }

    if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        close(fd); freeaddrinfo(res); return false;
    }
    freeaddrinfo(res);

    // Build HTTP/1.1 request
    std::string request =
        "POST " + path + " HTTP/1.1\r\n"
        "Host: " + host + ":" + std::to_string(port) + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    size_t sent = 0;
    while (sent < request.size()) {
        ssize_t n = send(fd, request.c_str() + sent,
                         request.size() - sent, 0);
        if (n <= 0) { close(fd); return false; }
        sent += static_cast<size_t>(n);
    }

    // Drain the response (we only care whether the POST succeeded)
    char buf[256];
    while (recv(fd, buf, sizeof(buf), 0) > 0) {}

    close(fd);
    return true;
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string serverHost = "localhost";
    int         serverPort = 3001;
    int         intervalMs = 2000;   // push interval in milliseconds

    // Parse simple CLI overrides: --host <h> --port <p> --interval <ms>
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "--host")     == 0) serverHost = argv[i + 1];
        if (std::strcmp(argv[i], "--port")     == 0) serverPort = std::stoi(argv[i + 1]);
        if (std::strcmp(argv[i], "--interval") == 0) intervalMs = std::stoi(argv[i + 1]);
    }

    std::cout << "[agent] System monitor started – pushing to "
              << serverHost << ":" << serverPort
              << " every " << intervalMs << " ms\n";

    SystemMonitor monitor;

    while (true) {
        try {
            SystemSnapshot snap = monitor.collect();
            std::string    json = SystemMonitor::toJson(snap);

            bool ok = httpPost(serverHost, serverPort, "/api/metrics", json);
            if (ok)
                std::cout << "[agent] pushed: cpu="
                          << static_cast<int>(snap.cpuUsagePercent)
                          << "% mem=" << static_cast<int>(snap.memUsedPercent)
                          << "%\n";
            else
                std::cerr << "[agent] failed to reach server – retrying\n";
        } catch (const std::exception& ex) {
            std::cerr << "[agent] error: " << ex.what() << "\n";
        }

        // Sleep for remainder of interval (collect() already sleeps ~0.5 s)
        int remaining = intervalMs - 500;
        if (remaining > 0)
            usleep(static_cast<useconds_t>(remaining) * 1000);
    }
}
