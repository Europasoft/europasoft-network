#pragma once
#include "Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

// TCP server connection listener op thread class
class ListenThread
{
protected:
    using size_t = std::size_t;

    std::mutex mutex;
    struct MutexMembers
    {
        // always call acquire() to initialize a lock object before accessing these members
        bool terminateThread = false;
        Sockets::MutexSocket socket{ INVALID_SOCKET };
    } mxm;
    using MXM = MutexMembers;

    // locks the mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) [[nodiscard]] std::unique_lock<std::mutex>&& getMxm(MXM*& mxmOut)
    {
        return std::move(std::unique_lock<std::mutex>(mutex));
        mxmOut = &mxm; // member resources acquired by calling thread
    }

public:
    std::thread thread{};
    bool threadRunning = false;

    ListenThread() = default;
    ~ListenThread() { terminateThread(); }

    void start(const std::string& port);
    virtual void threadMain(ListenThread* p);

    // returns each new connection socket once, in connection queue order, otherwise INVALID_SOCKET
    SOCKET getConnectionSocket();
    void terminateThread();

};



