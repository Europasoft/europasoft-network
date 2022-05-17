#include "SocketThread.h"
#include <cassert>

SocketStreamThread::SocketStreamThread(const size_t& sendBufferSize_, const size_t& receiveBufferSize_, 
                    const size_t sndMax, const size_t& recMax) : sndMaxSize{sndMax}, recMaxSize{recMax}
{
    reallocSendBuffer(sendBufferSize_);
    reallocReceiveBuffer(receiveBufferSize_);
}
SocketStreamThread::~SocketStreamThread()
{ 
    terminateThread();
    delete[] mxm.sndBuffer;
    delete[] mxm.recBuffer;
}

void SocketStreamThread::bufMemRealloc(char*& bptr, const size_t& newSize)
{
    assert(newSize > 0);
    MXM* mxm;
    auto lock = getMxm(mxm);
    if (bptr) { delete[] bptr; }
    bptr = new char[newSize];
}
void SocketStreamThread::reallocSendBuffer(const size_t& newSize) 
{ 
    bufMemRealloc(mxm.sndBuffer, newSize); 
    mxm.sndBufferSize = newSize; 
}
void SocketStreamThread::reallocReceiveBuffer(const size_t& newSize)
{
    bufMemRealloc(mxm.recBuffer, newSize);
    mxm.recBufferSize = newSize;
}

void SocketStreamThread::start(Sockets::MutexSocket* s)
{
    assert(s != nullptr && "stream thread requires valid pointer to mutex-protected socket");
    mxm.socket = s;
    thread = std::thread([this] { this->threadMain(this); }); // create thread
}

void SocketStreamThread::threadMain(SocketStreamThread* p)
{
    threadRunning = true;
    bool terminate = false;
    while (!terminate)
    {
        MXM* mxmptr; 
        auto lock = p->getMxm(mxmptr); // lock mutex protecting mxm structure
        MXM& mxm = *mxmptr;

        // send TCP stream data
        if (mxm.sndDataSize > 0)
        {
            Sockets::MutexSocket::Lock sLock;
            SOCKET s = mxm.socket->get(sLock); // lock socket mutex, released at end of block
            if (Sockets::sendData(s, mxm.sndBuffer, mxm.sndDataSize)) 
            { mxm.sndDataSize = 0; } // indicate data sent
        }
        assert(mxm.recDataSize == 0 && "previously received data still present, risk of information loss");
        // receive TCP stream data (blocking until output buffer is full)
        {
            Sockets::MutexSocket::Lock sLock;
            SOCKET s = mxm.socket->get(sLock); // lock socket mutex, released at end of block
            Sockets::RecStat r = Sockets::receiveData(s, mxm.recBuffer, mxm.recBufferSize);
            mxm.recDataSize = r.size;
            if (r.e == Sockets::RecStatE::ConnectionClosed)
            { mxm.terminateThread = true; } // remote peer closed the connection
        }
        terminate = mxm.terminateThread;
    }
    threadRunning = false;
}

bool SocketStreamThread::queueSend(const char& data, const size_t& size, bool overwrite)
{
    assert(threadRunning);
    MXM* mxm;
    auto lock = getMxm(mxm);
    if (size == 0 || (mxm->sndDataSize > 0 && !overwrite) || mxm->terminateThread) { return false; }
    if (size > mxm->sndBufferSize) 
    { 
        if (size > sndMaxSize) { return; }
        reallocSendBuffer(size); 
    }
    memcpy(mxm->sndBuffer, &data, size);
    mxm->sndDataSize = size; // indicate size of data to be sent
    return true;
}

bool SocketStreamThread::getReceiveBuffer(char& dstBuffer, const size_t& dstBufferSize)
{
    MXM* mxm;
    if (!mutex.try_lock()) { return false; }
    if (dstBufferSize < mxm->recDataSize || mxm->recDataSize == 0) { mutex.unlock(); return false; }
    memcpy(&dstBuffer, mxm->recBuffer, mxm->recDataSize);
    mxm->recDataSize = 0; // mark as empty ("was read")
    mutex.unlock();
}

void SocketStreamThread::terminateThread()
{
    MXM* mxm;
    auto lock = getMxm(mxm);
    mxm->terminateThread = true;
}