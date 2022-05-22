#pragma once
#include "Sockets/Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP server connection listener op thread class
class ListenThread
{
protected:
    std::thread thread{};
    bool threadRunning = false; // status value set by listen thread
    bool forceTerminate = false; // may be set by main thread

    std::string listenPort{};
    Sockets::MutexSocket connectedSocket;

public:
    using Lock = Sockets::Lock; // syntactic sugar
    ListenThread() = default;
    ~ListenThread() { terminateThread(); }

    void start(const std::string& port);
    void threadMain();

    bool isThreadRunning() const { return threadRunning; }
    // forces the stream thread to shut down
    void terminateThread() { forceTerminate = true; }

    /*  returns a new connection socket (or INVALID_SOCKET if no new connections)
        NOTE: returned sockets are forgotten, so they must be closed by the caller */
    [[nodiscard]] SOCKET getConnectionSocket();

};



