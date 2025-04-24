#pragma once
#include "Sockets/Sockets.h"
#include "NetThread/NetThreadSync.h"
#include <thread>
#include <chrono>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP send/receive op thread class
class StreamThread
{
public:
    using Lock = Sockets::Lock; // syntactic sugar
    StreamThread(size_t sendBufferSize, size_t receiveBufferSize);
    ~StreamThread();

    void start(std::string_view hostname_, std::string_view port_, size_t connectTimeout_);
    void start(SOCKET socket_);
    
    
    bool isStreamConnected() const { return streamConnected; };
    bool isFailed() const { return connectionFailure; }

    // thread-safely copies to send buffer, returns false if buffer still has unsent data
    bool queueSend(std::string_view data);

    void getReceiveBuffer(std::string& data);

    // thread-safely copies data from the receive buffer, then marks it as empty
    size_t getReceiveBuffer(char* dstBuffer, const size_t& dstBufferSize);

    // forces the stream thread to shut down
    void stop() { forceTerminate = true; }

protected:
    void threadMain();
    std::thread thread{};
    double connectTimeout = 1.0;
    std::atomic<bool> streamConnected = false;
    std::atomic<bool> connectionFailure = false;
    std::atomic<bool> forceTerminate = false;

    Sockets::MutexSocket socket;
    NetBuffer recvBuffer, sendBuffer;
    std::string hostname, port;
	Timer lastComTimer;
};


