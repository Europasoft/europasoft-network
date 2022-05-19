#include "StreamThread.h"
#include <cassert>
#include <limits.h>

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

StreamThread::StreamThread(const size_t& sendBufferSize, const size_t& receiveBufferSize,
                        const size_t sndMax, const size_t& recMax)
                        : sndBuffer{ sendBufferSize, sndMax }, recBuffer{ receiveBufferSize, recMax } {};
StreamThread::~StreamThread() { terminateThread(); }

void StreamThread::start(const std::string& hostName, const std::string& port_)
{
    // stream thread will handle connecting, since hostname was specified
    if (isThreadRunning()) { return; }
    hostname = hostName;
    port = port_;
    thread = std::thread([this] { this->threadMain(); }); // create thread
}
void StreamThread::start(const SOCKET& s)
{
    // this overload assumes socket is already connected
    if (isThreadRunning()) { return; }
    assert(s != INVALID_SOCKET && "connected socket (or hostname) required to start stream thread");
    socket.set(s);
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void StreamThread::threadMain()
{
    threadRunning = true;
    bool terminate = false;

    // resolve hostname and connect
    if (!socket.isInitialized())
    { 
        SOCKET s;
        auto r = Sockets::setupStream(hostname, port, s);
        if (!r) { terminate = true; } // failure to connect
        else 
        { socket.set(s); } // set function is threadsafe
    }

    // thread main loop
    while (!terminate)
    {
        if (disconnectOutgoing) 
        { 
            Lock sl; 
            disconnectOutgoing = !Sockets::shutdownConnection(socket.get(sl), 1); 
        }
        // send TCP stream data
        if (sndBuffer.getDataSize() > 0)
        {
            Lock s_lock, b_lock;
            SOCKET s = socket.get(s_lock); // lock socket mutex, released at end of block
            if (Sockets::sendData(s, sndBuffer.getBuffer(b_lock), sndBuffer.getDataSize()))
            { sndBuffer.setDataSize(0); } // indicate data sent (mark empty)
        }

        // receive TCP stream data (blocking until buffer is full)
        if (recBuffer.getDataSize() == 0)
        {
            Lock s_lock, b_lock;
            SOCKET s = socket.get(s_lock); // lock socket mutex, released at end of block
            Sockets::RecStat r = Sockets::receiveData(s, recBuffer.getBuffer(b_lock), recBuffer.getDataSize());
            recBuffer.setDataSize(r.size);
            if (r.e == Sockets::RecStatE::ConnectionClosed)
            { terminate = true; } // remote peer closed the connection
        }
        terminate = forceTerminate || terminate;
    }
    threadRunning = false;
}

bool StreamThread::queueSend(const char& data, const size_t& size, bool overwrite)
{
    assert(isThreadRunning() && "cannot send data, stream thread not running");
    assert(size <= sndBuffer.bufMax && "send data will not fit in buffer");
    if (size == 0 || (sndBuffer.getDataSize() > 0 && !overwrite) ||
        forceTerminate || !isThreadRunning()) { return false; }
    Lock bl; 
    auto* bptr = sndBuffer.getBuffer(bl); // lock mutex to safely modify the buffer
    if (size > sndBuffer.getDataSize())
    { 
        if (size > sndBuffer.bufMax) { return false; }
        sndBuffer.realloc(size);
    }
    memcpy(bptr, &data, size);
    sndBuffer.setDataSize(size); // indicate size of data to be sent
    return true;
}

size_t StreamThread::getReceiveBuffer(char& dstBuffer, const size_t& dstBufferSize)
{
    assert(dstBufferSize >= recBuffer.getDataSize() && "destination buffer too small, data will be lost");
    if (recBuffer.getDataSize() == 0) { return 0; }
    // copy as much as will fit
    const auto& datasize = (dstBufferSize > recBuffer.getDataSize()) ? recBuffer.getDataSize() : dstBufferSize;
    Lock bl;
    memcpy(&dstBuffer, recBuffer.getBuffer(bl), datasize);
    recBuffer.setDataSize(0); // mark as empty ("was read")
    return datasize;
}
