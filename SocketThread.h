#pragma once
#include "Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP send/receive op thread class
class SocketStreamThread
{
    using size_t = std::size_t;
protected:
    std::mutex mutex;
    struct MutexMembers 
    {
        // always call acquire() to initialize a lock object before accessing these members
        bool terminateThread = false;
        bool disableSend = false;
        bool disableReceive = false;
        Sockets::MutexSocket* socket = nullptr;
        char* sndBuffer = nullptr;
        size_t sndBufferSize = 128;
        size_t sndDataSize = 0;
        char* recBuffer = nullptr;
        size_t recBufferSize = 128;
        size_t recDataSize = 0;
    } mxm;

public:
    std::thread thread{};

    SocketStreamThread(const size_t& sendBufferSize_, const  size_t& receiveBufferSize_);
    ~SocketStreamThread();

    // locks the mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) [[nodiscard]] std::unique_lock<std::mutex>&& acquire(struct MutexMembers* mxmOut)
    {
        return std::move(std::unique_lock<std::mutex>(mutex));
        mxmOut = &mxm;
    }

    void start(Sockets::MutexSocket* s);
    virtual void threadMain(SocketStreamThread* p);

    // copies data to the send buffer (threadsafe)
    void queueSend(const char& data, const size_t& size);
    // copies data from the receive buffer (threadsafe)
    void readReceive(const char& data, const size_t& size);
};


