#pragma once
#include "Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

class SocketThread
{
    std::thread thread{};
    std::mutex mutex;
    // locks a mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) static [[nodiscard]] std::unique_lock<std::mutex>&& acquire(std::mutex& mutex)
    { return std::move(std::unique_lock<std::mutex>(mutex)); }
    // always call acquire() to initialize a lock object before accessing these members
    bool terminateThread = false;
    bool idleThread = false;
    char* buffer = nullptr;

    virtual void threadMain(SocketThread* p) { return; } // override this
public:
    SocketThread(size_t bufferSize = 512) : buffer{ new char[bufferSize] } {}
    ~SocketThread() { delete[] buffer; }
    void start() { thread = std::thread([this] { this->threadMain(this); }); }
};

class ReceiveThread : public SocketThread
{
    void threadMain(SocketThread* p) override 
    {

    }
};

class TransmitThread : public SocketThread
{

};

