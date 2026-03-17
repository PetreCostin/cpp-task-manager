#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "../task/task_manager.hpp"

// ─── Server ───────────────────────────────────────────────────────────────────
// A minimal TCP server that exposes task-manager status over the network.
// Clients connect and receive a plain-text or JSON summary, then the
// connection is closed.  The server runs on a background thread so it does
// not block the ncurses UI.
class Server {
public:
    explicit Server(int port = 9090);
    ~Server();

    // Start listening; pass a pointer to the shared TaskManager (must outlive
    // the Server object).  Returns true on success.
    bool start(const TaskManager* tm);

    // Signal the background thread to stop and wait for it to finish.
    void stop();

    bool isRunning() const;
    int  port()      const;

private:
    void listenLoop(const TaskManager* tm);
    std::string buildResponse(const TaskManager* tm) const;

    int  port_;
    int  serverFd_  = -1;
    std::thread        thread_;
    std::atomic<bool>  running_{false};
};
