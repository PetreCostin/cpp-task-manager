#include "server.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>

#include "../core/logger.hpp"

// ─── Construction / Destruction ───────────────────────────────────────────────
Server::Server(int port) : port_(port) {}

Server::~Server() {
    stop();
}

int  Server::port()      const { return port_; }
bool Server::isRunning() const { return running_.load(); }

// ─── Start / Stop ─────────────────────────────────────────────────────────────
bool Server::start(const TaskManager* tm) {
    if (running_) return false;

    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        Logger::instance().error(std::string("Server: socket() failed: ") + strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Non-blocking so accept() can be interrupted by the stop flag.
    fcntl(serverFd_, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        Logger::instance().error(std::string("Server: bind() failed: ") + strerror(errno));
        ::close(serverFd_);
        serverFd_ = -1;
        return false;
    }

    listen(serverFd_, 8);
    running_ = true;
    thread_  = std::thread(&Server::listenLoop, this, tm);
    Logger::instance().info("Server: listening on port " + std::to_string(port_));
    return true;
}

void Server::stop() {
    if (!running_) return;
    running_ = false;
    if (serverFd_ >= 0) {
        ::close(serverFd_);
        serverFd_ = -1;
    }
    if (thread_.joinable())
        thread_.join();
    Logger::instance().info("Server: stopped");
}

// ─── Background Listen Loop ───────────────────────────────────────────────────
void Server::listenLoop(const TaskManager* tm) {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t   addrLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_,
                              reinterpret_cast<sockaddr*>(&clientAddr),
                              &addrLen);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connection; sleep briefly and retry.
                usleep(100'000);
                continue;
            }
            if (running_)
                Logger::instance().error(std::string("Server: accept() error: ") + strerror(errno));
            break;
        }

        std::string response = buildResponse(tm);
        // Best-effort send; ignore partial-write edge cases for this stub.
        send(clientFd, response.c_str(), response.size(), 0);
        ::close(clientFd);
    }
}

// ─── Response Builder ─────────────────────────────────────────────────────────
std::string Server::buildResponse(const TaskManager* tm) const {
    if (!tm) return "HTTP/1.0 503 Service Unavailable\r\n\r\nNo data\n";

    std::ostringstream oss;
    oss << "HTTP/1.0 200 OK\r\n"
        << "Content-Type: text/plain\r\n"
        << "\r\n"
        << "C++ Task Manager – Status Report\n"
        << "=================================\n"
        << "Total tasks  : " << tm->tasks.size()                          << "\n"
        << "Pending      : " << tm->count(TaskStatus::PENDING)            << "\n"
        << "In Progress  : " << tm->count(TaskStatus::IN_PROGRESS)        << "\n"
        << "Done         : " << tm->count(TaskStatus::DONE)               << "\n"
        << "\nTasks:\n";

    for (const auto& t : tm->tasks) {
        const char* pStr = (t.priority == Priority::HIGH)   ? "HIGH"   :
                           (t.priority == Priority::MEDIUM) ? "MEDIUM" : "LOW";
        const char* sStr = (t.status == TaskStatus::PENDING)     ? "PENDING"     :
                           (t.status == TaskStatus::IN_PROGRESS)  ? "IN_PROGRESS" : "DONE";
        oss << "  [" << t.id << "] " << t.name
            << "  (" << pStr << " / " << sStr << ")\n";
    }

    return oss.str();
}
