#include "ListenThread.h"
#include <cassert>

void ListenThread::bufMemRealloc(char*& bptr, const size_t& newSize)
{
    assert(newSize > 0);
    MXM* mxm = nullptr;
    auto lock = getMxm(mxm);
    if (bptr) { delete[] bptr; }
    bptr = new char[newSize];
}
void ListenThread::reallocSendBuffer(const size_t& newSize)
{
    bufMemRealloc(mxm.sndBuffer, newSize);
    mxm.sndBufferSize = newSize;
}
void ListenThread::reallocReceiveBuffer(const size_t& newSize)
{
    bufMemRealloc(mxm.recBuffer, newSize);
    mxm.recBufferSize = newSize;
}

void ListenThread::start(Sockets::MutexSocket* s, const std::string& hostname_)
{
    hostname = hostname_; // stream thread will handle connecting, since hostname was specified
    start(s);
}
void ListenThread::start(Sockets::MutexSocket* s)
{
    // this overload assumes socket is already connected
    assert(s != nullptr && "stream thread needs pointer to mutex-protected socket");
    mxm.socket = s;
    thread = std::thread([this] { this->threadMain(this); }); // create thread
}

void ListenThread::threadMain(ListenThread* p)
{
    threadRunning = true;
    bool terminate = false;
    // resolve hostname and connect
    if (!hostname.empty())
    {
        SOCKET s;
        auto r = Sockets::setupStream(hostname, s);
        if (!r) { terminate = true; } // failure to connect
        else
        {
            MXM* mxm = nullptr;
            auto lock = p->getMxm(mxm);
            mxm->socket->set(s); // set function is threadsafe
        }
    }

    // thread main loop
    while (!terminate)
    {
        MXM* mxmptr = nullptr;
        auto lock = p->getMxm(mxmptr); // lock mutex protecting mxm structure
        MXM& mxm = *mxmptr;

        // send TCP stream data
        if (mxm.sndDataSize > 0)
        {
            Sockets::MutexSocket::Lock sLock;
            SOCKET s = mxm.socket->get(sLock); // lock socket mutex, released at end of block
            if (Sockets::sendData(s, mxm.sndBuffer, mxm.sndDataSize))
            {
                mxm.sndDataSize = 0;
            } // indicate data sent
        }

        // receive TCP stream data (blocking until buffer is full)
        if (mxm.recDataSize == 0)
        {
            Sockets::MutexSocket::Lock sLock;
            SOCKET s = mxm.socket->get(sLock); // lock socket mutex, released at end of block
            Sockets::RecStat r = Sockets::receiveData(s, mxm.recBuffer, mxm.recBufferSize);
            mxm.recDataSize = r.size;
            if (r.e == Sockets::RecStatE::ConnectionClosed)
            {
                mxm.terminateThread = true;
            } // remote peer closed the connection
        }
        terminate = mxm.terminateThread;
    }
    threadRunning = false;
}

SOCKET ListenThread::getConnectionSocket()
{
    SOCKET socketOut = INVALID_SOCKET;
    {
        MXM* mxm = nullptr;
        auto lock = getMxm(mxm);
        Sockets::MutexSocket::Lock socketLock;
        socketOut = mxm->socket.get(socketLock); // get
        mxm->socket.set(INVALID_SOCKET, true); // mark as "read"
    }
    return socketOut;
}

void ListenThread::terminateThread()
{
    MXM* mxm = nullptr;
    auto lock = getMxm(mxm);
    mxm->terminateThread = true;
}