// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include <mutex>
#include <chrono>
#include <cassert>

// data buffer wrapper, with built-in mutex management for multi-threaded access
class NetBuffer
{
public:
	using Lock = std::unique_lock<std::recursive_mutex>; // syntactic sugar
	NetBuffer(size_t allocSize = 0);
    NetBuffer(NetBuffer&& src) noexcept;
    NetBuffer(const NetBuffer& src);
    ~NetBuffer();

	// expands the allocation size if necessary, returns true if buffer size is >= newSize
	bool reserve(size_t newSize, const Lock& lock);

	// fills this buffer with data, expanding the buffer if necessary, buffer must be empty
	bool copyFrom(const char* data, size_t size, const Lock& lock);

    // threadsafe, sets the reported size of data in buffer
    void setDataSize(size_t newSize);

    /* ensures thread safety by locking the mutex (may block if already locked by another thread)
		mutex will unlock when the lock object is destroyed */
#ifdef _Acquires_lock_()
	_Acquires_lock_(lock)
#endif 
	char* getBuffer(Lock& lock)
    {
		lock = std::move(Lock(m));
		return buffer;
    }

	// getters (will not block)
    size_t getDataSize() const { return dataSize; }
    size_t getBufferSize() const { return bufferSize; }
	static size_t getBufMax();

protected:
	char* buffer = nullptr;
	size_t bufferSize = 0; // current buffer size, may be partially filled
	size_t dataSize = 0; // size of data currently in buffer, <= bufferSize

	std::recursive_mutex m;
	void free();

	void verifyLock(const Lock& lock) const;
};

class Timer
{
	using clock = std::chrono::steady_clock;
	using timePoint = clock::time_point;
public:
	Timer() = default;
	void start() { startTime = clock::now(); }
	double getElapsed() const
	{
		std::chrono::duration<double, std::milli> ms = (clock::now() - startTime);
		return (ms.count() / 1000.0);
	}
	double getElapsedMs() const
	{
		std::chrono::duration<double, std::milli> ms = (clock::now() - startTime);
		return ms.count();
	}
	bool checkTimeout(double max) 
	{
		if (getElapsed() >= max) 
		{ 
			start(); 
			return true; 
		}
		return false;
	}
private:
	timePoint startTime;
};