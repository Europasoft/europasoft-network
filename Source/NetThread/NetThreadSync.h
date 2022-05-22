#pragma once
#include <mutex>
#include <stack>
#include <cassert>

// data buffer wrapper, with built-in mutex management for multi-threaded access
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
    {
        lock = std::move(std::unique_lock<std::recursive_mutex>(m)); return buffer;
    } // mutex will unlock when lock object is destroyed

// getters (will not block)
    const size_t& getDataSize() const { return dataSize; }
    const size_t& getBufferSize() const { return bufferSize; }
};

class NetThreadErrorHandler 
{
    enum class SocketErrorReason {  };
};