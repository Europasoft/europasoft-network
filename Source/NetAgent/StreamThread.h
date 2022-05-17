#pragma once
#include "Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP send/receive op thread class
class SocketStreamThread
{
protected:
    using size_t = std::size_t;

    std::mutex mutex;
    struct MutexMembers 
    {
        // always call acquire() to initialize a lock object before accessing these members
        bool terminateThread = false;
        Sockets::MutexSocket* socket = nullptr;
        char* sndBuffer = nullptr;
        char* recBuffer = nullptr;
        // current buffer size, buffers may be partially filled
        size_t sndBufferSize = 0;
        size_t recBufferSize = 0;
        // size of data currently in buffer, less or equal to buffer size 
        size_t sndDataSize = 0;
        size_t recDataSize = 0;
    } mxm;
    using MXM = MutexMembers;

    void bufMemRealloc(char*& bptr, const size_t& newSize);
    void reallocSendBuffer(const size_t& newSize);
    void reallocReceiveBuffer(const size_t& newSize);

    // locks the mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) [[nodiscard]] std::unique_lock<std::mutex>&& getMxm(MXM*& mxmOut)
    {
        return std::move(std::unique_lock<std::mutex>(mutex));
        mxmOut = &mxm; // member resources acquired by calling thread
    }

public:
    const size_t sndMaxSize;
    const size_t recMaxSize;
    std::thread thread{};
    bool threadRunning = false;
    // if not empty, indicates that the stream thread is responsible for connecting and socket creation
    std::string hostname = std::string();

    SocketStreamThread(const size_t& sendBufferSize_ = 256, const size_t& receiveBufferSize_ = 256, 
                        const size_t sndMax = 1024, const size_t& recMax = 1024);
    ~SocketStreamThread();

    void start(Sockets::MutexSocket* s, const std::string& hostname_);
    void start(Sockets::MutexSocket* s);
    virtual void threadMain(SocketStreamThread* p);

    // thread-safely copies data to the send buffer
    bool queueSend(const char& data, const size_t& size, bool overwrite = false);
    // thread-safely copies data from the receive buffer, then marks it as empty
    size_t getReceiveBuffer(char& dstBuffer, const size_t& dstBufferSize);
    void terminateThread();

};


