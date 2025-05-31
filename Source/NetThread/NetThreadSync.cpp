// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetThreadSync.h"
#include "Sockets/Sockets.h"
#include <limits>
#include <cassert>
#include <cstring>
#include <new>
#include <stdexcept>

NetBufferAdvanced::NetBufferAdvanced(size_t allocSize)
{
	Lock lock = Lock(m);
    reserve(allocSize, lock);
}

NetBufferAdvanced::~NetBufferAdvanced()
{ 
    free();
}

void NetBufferAdvanced::free()
{ 
	// wipe buffer content before freeing, just in case
	if (buffer and bufferSize > 0)
		memset(buffer, 0x00, bufferSize);
	// deallocate buffer
    if (buffer) 
		delete[] buffer;
    buffer = nullptr;
    bufferSize = 0;
}

void NetBufferAdvanced::reserve(size_t required, const Lock& lock)
{
	verifyLock(lock);
    if (required <= unwritten())
		{ return; }

    // allocate a new buffer, copy existing data (if any), replace old buffer
	const size_t newSize = unread() + required;
    auto* newBuffer = new(std::nothrow) char[newSize + sizeof(ES_CANARY_VALUE_U8)];
	newBuffer[newSize] = ES_CANARY_VALUE_U8;
	if (not newBuffer)
		{ throw std::runtime_error("alloc fail"); }

	if (unread() > 0 and buffer)
	{
		assert((newBuffer + unread()) <= newBuffer + newSize);
		memcpy(newBuffer, buffer + readPos, unread());
	}
    if (buffer)
		{ delete[] buffer; }
    buffer = newBuffer;
    bufferSize = newSize;
	writePos = unread();
	readPos = 0;
}

void NetBufferAdvanced::written(size_t opSize)
{
	verifyRange(writePos, opSize);
	writePos += opSize;
	if (writePos > bufferSize)
		throw std::runtime_error("buffer overflow");
}

void NetBufferAdvanced::read(size_t opSize)
{
	verifyRange(readPos, opSize);
	readPos += opSize;
	assert(readPos <= bufferSize and readPos <= writePos);
	if (readPos == writePos)
	{
		memcpy(buffer, (buffer + writePos), (bufferSize - writePos));
		readPos = 0;
		writePos = 0;
	}
}

void NetBufferAdvanced::verifyLock(const Lock& lock) const
{
	if (not lock.owns_lock() and lock.mutex() == &m)
		throw std::runtime_error("Invalid or missing lock for NetBuffer");
}

void NetBufferAdvanced::verifyRange(size_t startOffset, size_t size)
{
	assert((startOffset == readPos) or (startOffset == writePos));
	if ((startOffset + size) > bufferSize)
		throw std::runtime_error("range exceeded buffer size");
	const uint8_t canary = *((uint8_t*)(buffer + bufferSize));
	assert(canary == ES_CANARY_VALUE_U8);
	if (canary != ES_CANARY_VALUE_U8)
		throw std::runtime_error("buffer canary value was incorrect, possible buffer overflow");
}

NetBufferView::~NetBufferView()
{
	viewReportDestroyed();
	const bool accessFail = (wasValid and not accessPerformed);
	assert((not accessFail) && "buffer view was destroyed before data access was registered");
	if (accessFail)
	{
		std::abort();
	}
}
void NetBufferView::verifyBufferRange()
{
	assert(parentBuffer != nullptr);
	parentBuffer->verifyView(*this);
}
void NetBufferView::viewReportDestroyed()
{
	if (parentBuffer != nullptr)
		parentBuffer->decrementViewCount();
}
