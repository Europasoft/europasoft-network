#include "SocketThread.h"
#include <stdexcept>

SocketStreamThread::SocketStreamThread(const size_t& sendBufferSize_, const  size_t& receiveBufferSize_)
{
    mxm.sndBufferSize = sendBufferSize_;
    mxm.recBufferSize = receiveBufferSize_;
    mxm.sndBuffer = new char[mxm.sndBufferSize];
    mxm.recBuffer = new char[mxm.recBufferSize];
}
SocketStreamThread::~SocketStreamThread()
{ 
    delete[] mxm.sndBuffer; 
    delete[] mxm.recBuffer; 
}

void SocketStreamThread::start(Sockets::MutexSocket* s)
{
    if (!s) { throw std::runtime_error("cannot start socket op thread, null socket handle pointer"); }
    mxm.socket = s;
    thread = std::thread([this] { this->threadMain(this); }); // create thread
}

void SocketStreamThread::threadMain(SocketStreamThread* p)
{
    bool terminate = false;
    while (!terminate)
    {
        SocketStreamThread::MutexMembers* mxm;
        auto lock = p->acquire(mxm); // lock mutex protecting mxm structure
        Sockets::RecStat opResult;
        // send TCP stream data
        if (!mxm->disableSend && !mxm->sndBufferEmpty) 
        {
            Sockets::MutexSocket::Lock socketLock;
            SOCKET s = mxm->socket->get(socketLock); // lock socket mutex, released at end of block
            opResult = Sockets::sendData(s, mxm->sndBuffer, mxm->sndBufferSize);
        }
        // receive TCP stream data
        if (!mxm->disableReceive)
        {
            Sockets::MutexSocket::Lock socketLock;
            SOCKET s = mxm->socket->get(socketLock); // lock socket mutex, released at end of block
            opResult = Sockets::receiveData(s, mxm->recBuffer, mxm->recBufferSize);
        }
        terminate = mxm->terminateThread;
    }
}

void SocketStreamThread::queueSend(const char& data, const size_t& size) 
{
    SocketStreamThread::MutexMembers* mxm;
    auto lock = acquire(mxm); // lock mutex protecting mxm structure
    if (size > mxm->sndBufferSize) { throw std::runtime_error("failed to queue data, send buffer too small"); }

}

void SocketStreamThread::readReceive(const char& data, const size_t& size)
{

}