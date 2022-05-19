#pragma once
#include "Sockets.h"
#include <thread>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif
class NetBuffer 
{
    char* buffer = nullptr;
    size_t bufferSize = 0; // current buffer size, may be partially filled
    size_t dataSize = 0; // size of data currently in buffer, less or equal to buffer size
public:
    std::recursive_mutex m;
    const size_t bufMax;
    NetBuffer();
    NetBuffer(const size_t& allocSize);
    NetBuffer(const size_t& allocSize, const size_t& max_);
    NetBuffer(NetBuffer&& src) noexcept; // move ctor
    NetBuffer(const NetBuffer& src); // copy ctor
    ~NetBuffer();
    void realloc(const size_t& newSize);
    // threadsafe, sets the reported size of data in buffer
    void setDataSize(const size_t& newSize);

    // ensures thread safety by locking the mutex (may block if already locked by another thread)
    _Acquires_lock_(lock) char* getBuffer(std::unique_lock<std::recursive_mutex>& lock)
    { lock = std::move(std::unique_lock<std::recursive_mutex>(m)); return buffer; } // mutex will unlock when lock object is destroyed

    // getters (will not block)
    const size_t& getDataSize() const { return dataSize; }
    const size_t& getBufferSize() const { return bufferSize; }
};

// TCP send/receive op thread class
class StreamThread
{
protected:
    using size_t = std::size_t;
    
    std::thread thread{};
    bool threadRunning = false; // status value set by stream thread
    bool forceTerminate = false; // may be set by main thread
    bool disconnectOutgoing = false; // orders a shutdown of the send connection, may be set by main thread
    
    Sockets::MutexSocket socket;
    NetBuffer recBuffer;
    NetBuffer sndBuffer;
    // if not empty, indicates that the stream thread is responsible for connecting and socket creation
    std::string hostname = std::string();
    std::string port = std::string();

    /* locks the mutex, blocks if currently locked by other thread, mutex will unlock when lock object is destroyed
    _Acquires_lock_(return) [[nodiscard]] std::unique_lock<std::mutex>&& getMxm(MXM*& mxmOut)
    {
        return std::move(std::unique_lock<std::mutex>(mutex));
        mxmOut = &mxm; // member resources acquired by calling thread
    } */

public:
    using Lock = Sockets::Lock; // syntactic sugar
    StreamThread(const size_t& sendBufferSize = 256, const size_t& receiveBufferSize = 256, 
                        const size_t sndMax = 1024, const size_t& recMax = 1024);
    ~StreamThread();
    
    void start(const std::string& hostName, const std::string& port_);
    void start(const SOCKET& s);
    void threadMain();
    bool isThreadRunning() const { return threadRunning; }

    // thread-safely copies data to the send buffer
    bool queueSend(const char& data, const size_t& size, bool overwrite = false);

    // thread-safely copies data from the receive buffer, then marks it as empty
    size_t getReceiveBuffer(char& dstBuffer, const size_t& dstBufferSize);

    void shutdownOutgoing() { disconnectOutgoing = true; }
    // forces the stream thread to shut down
    void terminateThread() { forceTerminate = true; }
    
};


