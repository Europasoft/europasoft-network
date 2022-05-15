#pragma once
#include "Sockets.h"
#include <thread>
#include <cassert>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

class SocketThread
{
public:
    SocketThread(size_t bufferSize_) : buffer{ new char[bufferSize_] }, bufferSize{bufferSize_} {}
    ~SocketThread() { delete[] buffer; }

    std::thread thread{};
    std::mutex mutex;
    // locks a mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) static [[nodiscard]] std::unique_lock<std::mutex>&& acquire(std::mutex& mutex)
    { return std::move(std::unique_lock<std::mutex>(mutex)); }
    // always call acquire() to initialize a lock object before accessing these members
    bool terminateThread = false;
    bool idleThread = false;
    char* buffer = nullptr;
    size_t bufferSize = 128;
    bool bufferEmpty = true;
    Sockets::MutexSocket* socket = nullptr;

    void start(Sockets::MutexSocket* s) 
    { 
        assert(s && "cannot start socket op thread, null socket pointer");
        socket = s;
        thread = std::thread([this] { this->threadMain(this); }); 
    }
    virtual void threadMain(SocketThread* p) { return; } // override this
};

class ReceiveThread : public SocketThread
{
public:
    void threadMain(SocketThread* p) override 
    {
        bool terminate = false;
        while (!terminate) 
        {
            auto lock = acquire(p->mutex);
            // record incoming TCP stream data
            auto result = lockReceiveUnlock(p->socket, p->buffer, p->bufferSize);
            terminate = p->terminateThread;
            if (p->idleThread) { while (p->idleThread) {} }
        }
    }

protected:
    static Sockets::RecStat lockReceiveUnlock(Sockets::MutexSocket* mxs, char* buffer, size_t bufferSize)
    {
        Sockets::MutexSocket::Lock lock;
        auto sock = mxs->get(lock); // mutex lock will be released at the end of this function
        return Sockets::receiveData(sock, *buffer, bufferSize);
    }
};

class TransmitThread : public SocketThread
{

};

