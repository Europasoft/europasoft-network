#pragma once
#include "Sockets/Sockets.h"
#include "NetThread/NetThreadSync.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP send/receive op thread class
class StreamThread
{
protected:
    using size_t = std::size_t;
    
    std::thread thread{};
    bool streamConnected = false;
    bool forceTerminate = false; // may be set by main thread
    
    Sockets::MutexSocket socket;
    NetBuffer recBuffer;
    NetBuffer sndBuffer;
    // if not empty, indicates that the stream thread is responsible for connecting and socket creation
    std::string hostname = std::string();
    std::string port = std::string();

public:
    using Lock = Sockets::Lock; // syntactic sugar
    StreamThread(const size_t& sendBufferSize = 256, const size_t& receiveBufferSize = 256, 
                        const size_t sndMax = 1024, const size_t& recMax = 1024);
    ~StreamThread();
    
    void start(const std::string& hostName, const std::string& port_);
    void start(const SOCKET& s);
    void threadMain();
    bool isStreamConnected() const { return streamConnected; };

    // thread-safely copies to send buffer (returns false if buffer still has unsent data, unless overwrite=true)
    bool queueSend(const char* data, const size_t& size, bool overwrite = false);

    // thread-safely copies data from the receive buffer, then marks it as empty
    size_t getReceiveBuffer(char* dstBuffer, const size_t& dstBufferSize);

    // forces the stream thread to shut down
    void terminateThread() { forceTerminate = true; }
};


