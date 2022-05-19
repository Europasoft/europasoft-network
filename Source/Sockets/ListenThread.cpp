#include "ListenThread.h"
#include <cassert>

void ListenThread::start(const std::string& port)
{
    listenPort = port;
    assert(!listenPort.empty() && "valid port number must be provided for listen socket");
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void ListenThread::threadMain()
{
    threadRunning = true;
    bool terminate = false;

    SOCKET listenSocket = INVALID_SOCKET;

    // create listen socket
    if (!Sockets::listenSocket(listenSocket, listenPort)) { terminate = true; }

    // listener loop
    while (!terminate)
    {
        Lock sl;
        // prevent overwriting connection socket before it has been read (presumably managed somewhere else)
        if (connectedSocket.get(sl) != INVALID_SOCKET) { continue; }
        SOCKET cs = accept(listenSocket, nullptr, nullptr); // try to accept a connection
        if (cs != INVALID_SOCKET) { connectedSocket.set(cs, true); }
        terminate = forceTerminate || terminate;
    }
    if (listenSocket != INVALID_SOCKET) { close(listenSocket); }
    threadRunning = false;
}

SOCKET ListenThread::getConnectionSocket()
{
    SOCKET socketOut = INVALID_SOCKET;
    {
        Lock sl;
        socketOut = connectedSocket.get(sl); // get
        connectedSocket.set(INVALID_SOCKET, true); // mark as "read"
    }
    return socketOut;
}
