#pragma once
#include "Sockets/Sockets.h"
#include <thread>
#include <vector>
#include <string>

// TCP server connection listener op thread class
class ListenThread
{
public:
    using Lock = Sockets::Lock; // syntactic sugar
    ListenThread() = default;
    ~ListenThread();

    void start(std::string_view port);
    
    void stop() { forceTerminate = true; } // forces the listen thread to shut down

    [[nodiscard]] std::vector<SOCKET> getConnectedSockets(); // threadsafe non-blocking, consumes returned elements

protected:
    void threadMain();
    std::thread thread;
    std::string listenPort{};
    std::atomic<bool> forceTerminate = false; // may be set by other thread
    
    void addConnectedSocket(SOCKET s);
    std::vector<SOCKET> connSockets; // socket connections ready to hand over
    std::recursive_mutex connSocketsMutex;
};