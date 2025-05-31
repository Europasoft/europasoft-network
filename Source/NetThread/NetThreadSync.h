// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include <mutex>
#include <chrono>
#include <cassert>
#include <stdexcept>
#include <cstdlib>

#include <iostream>

#define ESMax(a,b) (((a) > (b)) ? (a) : (b))
#define ESMin(a,b) (((a) < (b)) ? (a) : (b))

constexpr uint8_t ES_CANARY_VALUE_U8 = 85;

// buffer views mercilessly enforce correct access to a buffer: check before access, only access once, notify after access
class NetBufferView
{
private:
	friend class NetBufferAdvanced_buggy;

	char* addr = nullptr;
	size_t size = 0;
	size_t originalSize = 0;
	NetBufferAdvanced_buggy* parentBuffer = nullptr;
	std::unique_lock<std::recursive_mutex> lock{};

	bool wasValid = false;
	bool allowedForWrite = false;
	bool validityCheckPerformed = false;
	bool accessPerformed = false;

	NetBufferView(char* addrIn, size_t sizeIn, bool allowWrite, std::unique_lock<std::recursive_mutex>&& bufferLock, NetBufferAdvanced_buggy* buffer)
		: lock{ std::move(bufferLock) }, parentBuffer{ buffer }
	{
		addr = addrIn;
		size = sizeIn;
		originalSize = sizeIn;
		wasValid = ((addrIn != nullptr) and (sizeIn > 0));
		allowedForWrite = allowWrite;
	}

	NetBufferView()
	{
		wasValid = false;
	}

	void verifyBufferRange();
	void viewReportDestroyed();

public:
	~NetBufferView();

	NetBufferView(const NetBufferView&) = delete;
	NetBufferView& operator=(const NetBufferView&) = delete;
	NetBufferView(NetBufferView&&) = default;
	NetBufferView& operator=(NetBufferView&&) = default;
	
	bool canAccess()
	{
		validityCheckPerformed = true;
		return (wasValid and (addr != nullptr) and (size > 0));
	}
	
	operator bool() noexcept
	{ 
		return canAccess();
	}

	char* getPtr()
	{
		assert(lock.owns_lock() && "buffer view must own the lock");
		if (accessPerformed)
		{
			throw std::runtime_error("attempted to get address from a buffer view after it was already closed");
		}
		else if (not validityCheckPerformed)
		{
			throw std::runtime_error("attempted to get address from a buffer view without checking validity first");
		}
		if (wasValid and (addr != nullptr))
		{
			verifyBufferRange();
			auto* p = addr;
			addr = nullptr;
			return p;
		}
		else
		{
			throw std::runtime_error("attempted to get address from a nonvalid buffer view");
		}
	}

	size_t getSize()
	{
		assert(lock.owns_lock() && "buffer view must own the lock");
		if (accessPerformed)
		{
			throw std::runtime_error("attempted to get size from a buffer view after it was already closed");
		}
		else if (not validityCheckPerformed)
		{
			throw std::runtime_error("attempted to get size from a buffer view without checking validity first");
		}
		if (wasValid and (size > 0))
		{
			verifyBufferRange();
			auto s = size;
			size = 0;
			return s; 
		}
		else
		{
			throw std::runtime_error("attempted to get size from a nonvalid buffer view");
		}
	}

	// must be called after getting getPtr() and getSize(), when finished using the data
	void accessReadFinished(size_t sizeRead)
	{
		if (sizeRead > originalSize)
		{
			throw std::runtime_error("buffer overread or incorrect size reported to buffer view");
		}
		if (not accessPerformed)
		{
			accessPerformed = true;
			if ((size > 0) or (addr != nullptr))
			{
				throw std::runtime_error("reported access to a buffer view before all data was accessed");
			}
		}
		else
		{
			throw std::runtime_error("reported access to the same buffer view multiple times");
		}
	}

	void accessWriteFinished(size_t sizeWritten)
	{
		if (sizeWritten > originalSize)
		{
			throw std::runtime_error("buffer overflow or incorrect size reported to buffer view");
		}
		if (allowedForWrite and (not accessPerformed))
		{
			accessPerformed = true;
			if ((size > 0) or (addr != nullptr))
			{
				throw std::runtime_error("reported access to a buffer view before all data was accessed");
			}
		}
		else if (not allowedForWrite)
		{
			throw std::runtime_error("reported write access to a buffer view on which writing is not allowed");
		}
		else
		{
			throw std::runtime_error("reported access to the same buffer view multiple times");
		}
	}

	void bufferWrite(std::string_view str)
	{
		if (not allowedForWrite)
			throw std::runtime_error("attempted to write data into a read-only buffer view");
		if (str.size() > size)
			throw std::runtime_error("attempted to write more data than allowed by buffer view");
		std::memcpy(getPtr(), str.data(), str.size());
		accessWriteFinished(str.size());
	}

	bool bufferRead(std::string& str, size_t lengthMin, size_t lengthMax)
	{
		const size_t available = getSize();
		if (available < lengthMin)
			return false;
		const size_t lengthToCopy = ESMin(available, lengthMax);
		str += std::string(getPtr(), lengthToCopy);
		accessReadFinished(lengthToCopy);
		return true;
	}
};

class NetBufferAdvanced_buggy
{
private:
	std::vector<char> buffer;
	size_t readPos = 0;   // next unread byte
	size_t writePos = 0;   // next free byte
	size_t numViews = 0; // reference count
	mutable std::recursive_mutex mutex{};
	size_t bufferAccessCount = 0;

	char* getWriteAddr() { return buffer.data() + writePos; }
	char* getReadAddr() { return buffer.data() + readPos; }
	size_t getSizeWritable() { return buffer.size() - writePos; }
	size_t getSizeReadable() { return buffer.size() - readPos; }

	bool reserve(size_t required, const std::unique_lock<std::recursive_mutex>& lock)
	{
		assert(lock.owns_lock());
		if (not lock.owns_lock())
			return false;
		if (getSizeWritable() > required)
			return true;
		assert(numViews == 0);
		const size_t readable = (writePos - readPos);
		const size_t sizeNew = (readable + required);
		std::vector<char> bufferCpy = buffer;
		buffer = std::vector<char>(sizeNew);
		std::cout << "\nallocated new buffer of size " << sizeNew << "\n";
		std::memcpy(buffer.data(), bufferCpy.data() + readPos, readable);
		writePos -= readPos;
		readPos = 0;
		return true;
	}

public:
	explicit NetBufferAdvanced_buggy(size_t initialSize = 4096) 
		: buffer(initialSize) {}

	NetBufferAdvanced_buggy(const NetBufferAdvanced_buggy&) = delete;
	NetBufferAdvanced_buggy& operator=(const NetBufferAdvanced_buggy&) = delete;
	NetBufferAdvanced_buggy(NetBufferAdvanced_buggy&&) = delete;
	NetBufferAdvanced_buggy& operator=(NetBufferAdvanced_buggy&&) = delete;

	NetBufferView getViewForWrite(size_t willWriteSize)
	{
		std::unique_lock<std::recursive_mutex> lock(mutex); // lock the buffer
		assert(numViews == 0);
		if (willWriteSize <= 0 or numViews != 0)
			return NetBufferView();

		if (willWriteSize > getSizeWritable())
		{
			if (not reserve(willWriteSize, lock))
				return NetBufferView();
		}

		auto* viewAddr = getWriteAddr();
		size_t viewSize = ESMin(willWriteSize, getSizeWritable());

		std::cout << "\n!intending to write " << viewSize << " bytes to buffer view" << "!\n" << std::flush;
		std::cout << "\n!buf access count is " << bufferAccessCount << "!\n" << std::flush;
		writePos += viewSize;
		numViews++;
		bufferAccessCount++;
		return NetBufferView(viewAddr, viewSize, true, std::move(lock), this);
	}

	NetBufferView getViewForRead(size_t willReadSize)
	{
		std::unique_lock<std::recursive_mutex> lock(mutex); // lock the buffer
		assert(numViews == 0);
		if (willReadSize <= 0 or getSizeReadable() <= 0 or numViews != 0)
			return NetBufferView();

		auto* viewAddr = getReadAddr();
		size_t viewSize = ESMin(willReadSize, getSizeReadable());

		std::cout << "\n!intending to read " << viewSize << " bytes from buffer view" << "!\n" << std::flush;
		std::cout << "\n!buf access count is " << bufferAccessCount << "!\n" << std::flush;
		readPos += viewSize;
		if (readPos == writePos)
		{
			readPos = 0;
			writePos = 0;
		}
		numViews++;
		bufferAccessCount++;
		return NetBufferView(viewAddr, viewSize, false, std::move(lock), this);
	}

	// the size reported may be misleading if another thread changes it right after, acquire a view and check again
	size_t getSizeToRead() const
	{
		std::unique_lock<std::recursive_mutex> lock(mutex); // lock the buffer (temporarily)
		assert(numViews == 0);
		size_t sizeOut = (buffer.size() - readPos);
		return sizeOut;
	}

	void verifyView(NetBufferView& view)
	{
		auto* viewStart = view.addr;
		auto* viewEnd = view.addr + view.size;
		assert(viewStart >= buffer.data());
		assert(viewEnd <= buffer.data() + buffer.size());
	}

	void decrementViewCount()
	{
		numViews--;
		assert(numViews >= 0);
	}
};

class NetBufferAdvanced
{
public:
	using Lock = std::unique_lock<std::recursive_mutex>; // syntactic sugar
	NetBufferAdvanced(size_t allocSize = 512);
	NetBufferAdvanced(NetBufferAdvanced&&) = delete;
	NetBufferAdvanced(const NetBufferAdvanced&) = delete;
	~NetBufferAdvanced();

	/* ensures thread safety by locking the mutex (may block if already locked by another thread)
		mutex will unlock when the lock object is destroyed */
#ifdef _Acquires_lock_()
	_Acquires_lock_(lock)
#endif 
	char* getBufferForRead(Lock& lock, size_t& sizeReadable)
	{
		lock = std::move(Lock(m));
		verifyLock(lock);
		sizeReadable = unread();
		return buffer + readPos;
	}

#ifdef _Acquires_lock_()
	_Acquires_lock_(lock)
#endif 
	char* getBufferForWrite(Lock& lock, size_t toBeWritten)
	{
		lock = std::move(Lock(m));
		verifyLock(lock);
		reserve(toBeWritten, lock);
		return buffer + writePos;
	}

#ifdef _Acquires_lock_()
	_Acquires_lock_(lock)
#endif 
	size_t peekReadSize(Lock& lock) const
	{
		lock = std::move(Lock(m));
		verifyLock(lock);
		return unread();
	}

	void written(size_t opSize);

	void read(size_t opSize);
	

protected:
	char* buffer = nullptr;
	size_t bufferSize = 0; // current buffer size, may be partially filled
	
	size_t readPos = 0;
	size_t writePos = 0;
	mutable std::recursive_mutex m;

	// expands the allocation size without discarding unread data
	void reserve(size_t required, const Lock& lock);
	void free();

	void verifyLock(const Lock& lock) const;
	void verifyRange(size_t startOffset, size_t size);

	size_t unread() const { return (writePos - readPos); }
	size_t unwritten() const { return (bufferSize - writePos); }

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
