#pragma once
#include "Sockets/Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif
class StreamThread;

// TCP server connection listener op thread class
class ListenThread
{
protected:
    std::thread thread{};
    bool forceTerminate = false; // may be set by main thread

    std::string listenPort{};
    StreamThread* streamThreadPtr = nullptr; // used to automatically hand over the first connected socket

public:
    using Lock = Sockets::Lock; // syntactic sugar
    ListenThread(StreamThread& streamThread);
    ~ListenThread();

    void start(const std::string& port);
    void threadMain();

    // forces the stream thread to shut down
    void terminateThread() { forceTerminate = true; }
};



