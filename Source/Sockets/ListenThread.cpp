#include "ListenThread.h"
#include <cassert>

void ListenThread::start(const std::string& port)
{
    mxm.listenPort = port;
    assert(!mxm.listenPort.empty() && "valid port number must be provided for listen socket");
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void ListenThread::threadMain()
{
    threadRunning = true;
    bool terminate = false;

    SOCKET listenSocket = INVALID_SOCKET;
    std::string lp;
    {
        // thread-safely get the listen port
        MXM* mxmptr = nullptr;
        auto lock = getMxm(mxmptr);
        lp = mxmptr->listenPort;
    }
    // create listen socket
    if (!Sockets::listenSocket(listenSocket, lp)) { terminate = true; }

    // listener loop
    while (!terminate)
    {
        MXM* mxmptr = nullptr;
        auto lock = getMxm(mxmptr); // lock mutex protecting mxm structure
        MXM& mxm = *mxmptr;
        // prevent overwriting connection socket before it has been saved somewhere else!
        if (mxm.connectedSocket != INVALID_SOCKET)  
        SOCKET cs = accept(listenSocket, NULL, NULL); // try to accept any connection
        if (cs == INVALID_SOCKET) { continue; }
        mxmptr->connectedSocket = cs;
    }
    if (listenSocket != INVALID_SOCKET) { close(listenSocket); }
    threadRunning = false;
}

SOCKET ListenThread::getConnectionSocket()
{
    SOCKET socketOut = INVALID_SOCKET;
    {
        MXM* mxm = nullptr;
        auto lock = getMxm(mxm);
        socketOut = mxm->connectedSocket; // get
        mxm->connectedSocket = INVALID_SOCKET; // mark as "read"
    }
    return socketOut;
}

void ListenThread::terminateThread()
{
    MXM* mxm = nullptr;
    auto lock = getMxm(mxm);
    mxm->terminateThread = true;
}