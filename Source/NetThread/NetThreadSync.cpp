#include "NetThreadSync.h"
#include "Sockets/Sockets.h"
#include <limits>
#include <cassert>
#include <cstring>

NetBuffer::NetBuffer(size_t allocSize)
{
    std::unique_lock<std::recursive_mutex> lock;
    getBuffer(lock);
    reserve(allocSize, lock);
}

NetBuffer::NetBuffer(NetBuffer&& src) noexcept
{
    buffer = src.buffer;
    bufferSize = src.bufferSize;
    dataSize = src.dataSize;
    src.buffer = nullptr;
    src.bufferSize = src.dataSize = 0;
}

NetBuffer::NetBuffer(const NetBuffer& src)
{
    if (!src.buffer && !src.dataSize && !src.bufferSize) { return; }
    std::unique_lock<std::recursive_mutex> lock;
    getBuffer(lock);
    reserve(src.bufferSize, lock);
    copyFrom(src.buffer, src.dataSize, lock);
}

NetBuffer::~NetBuffer() 
{ 
    free();
}

void NetBuffer::free()
{ 
    if (buffer) { delete[] buffer; }
    buffer = nullptr;
    bufferSize = dataSize = 0;
}

size_t NetBuffer::getBufMax() { return (std::numeric_limits<size_t>::max)(); }

bool NetBuffer::reserve(size_t newSize, const Lock& lock)
{
    assert(verifyLock(lock));
    if (newSize > getBufMax()) { return false; }
    if (newSize <= bufferSize) { return true; }
    // allocate a new buffer, copy existing data (if any), replace old buffer
    auto* newBuffer = new char[newSize]; 
    memcpy(newBuffer, buffer, dataSize);
    if (buffer) { delete[] buffer; }
    buffer = newBuffer;
    bufferSize = newSize;
    return true;
}

bool NetBuffer::copyFrom(const char* data, size_t size, const Lock& lock)
{
    assert(verifyLock(lock));
    assert(!dataSize && "copying into buffer would overwrite existing data");
    if (dataSize || !data || !size) { return false; }

    if (!reserve(size, lock)) { return false; }
    memcpy(buffer, data, size);
    dataSize = size;
    return true;
}

void NetBuffer::setDataSize(size_t newSize)
{
    Sockets::Lock lock; 
    getBuffer(lock);
    dataSize = newSize;
};

