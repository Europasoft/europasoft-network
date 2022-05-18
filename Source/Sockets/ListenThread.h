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
    std::mutex mutex;
    struct MutexMembers
    {
        // always call acquire() to initialize a lock object before accessing these members
        bool terminateThread = false;
        std::string listenPort{};
        SOCKET connectedSocket = INVALID_SOCKET;
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
    void threadMain();

    /*  returns a new connection socket (or INVALID_SOCKET if no new connections)
        NOTE: returned sockets are forgotten, so they must be closed by caller! */
    [[nodiscard]] SOCKET getConnectionSocket();
    void terminateThread();

};



