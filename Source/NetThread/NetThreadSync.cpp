#include "NetThreadSync.h"
#include "Sockets/Sockets.h"
#include <cassert>

NetBuffer::NetBuffer() : bufMax{ SIZE_MAX } {};
NetBuffer::NetBuffer(const size_t& allocSize) : bufMax{ SIZE_MAX } { realloc(allocSize); }
NetBuffer::NetBuffer(const size_t& allocSize, const size_t& max_) : bufMax{ max_ } { realloc(allocSize); }
NetBuffer::NetBuffer(NetBuffer&& src) noexcept : bufMax{ src.bufMax }
{
    buffer = src.buffer;
    bufferSize = src.bufferSize;
    dataSize = src.dataSize;
    src.buffer = nullptr;
    src.dataSize = 0;
}
NetBuffer::NetBuffer(const NetBuffer& src) : bufMax{ src.bufMax }
{
    dataSize = 0;
    realloc(src.bufferSize);
    memcpy(buffer, src.buffer, src.bufferSize);
    dataSize = src.dataSize;
    bufferSize = src.bufferSize;
}
NetBuffer::~NetBuffer() { if (buffer) { delete[] buffer; } }
void NetBuffer::realloc(const size_t& newSize)
{
    assert(newSize > 0 && newSize <= bufMax && (newSize <= dataSize || dataSize == 0) && "invalid buffer size");
    if (newSize == 0 || bufferSize == newSize) { return; }
    Sockets::Lock bl; getBuffer(bl); // lock mutex to safely modify the buffer
    bufferSize = min(newSize, bufMax);
    auto* dst = new char[bufferSize];
    if (dataSize > 0 && buffer) { memcpy(dst, buffer, dataSize); }
    if (buffer) { delete[] buffer; }
    buffer = dst;
}
void NetBuffer::setDataSize(const size_t& newSize)
{
    Sockets::Lock bl; getBuffer(bl);
    dataSize = newSize;
};

