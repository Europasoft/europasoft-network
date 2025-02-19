#include "ListenThread.h"
#include "StreamThread.h"
#include <cassert>

ListenThread::~ListenThread() 
{ 
    stop();
    thread.join();
}

void ListenThread::start(std::string_view port)
{
    listenPort = port;
    assert(!listenPort.empty() && "valid port number must be provided for listen socket");
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void ListenThread::threadMain()
{
    SOCKET listenSocket = INVALID_SOCKET;
    forceTerminate = !Sockets::createListenSocket(listenSocket, listenPort);

    // listener loop
    while (!forceTerminate)
    {
        SOCKET s = accept(listenSocket, nullptr, nullptr); // try to accept a connection
        if (s != INVALID_SOCKET) { addConnectedSocket(s); }
    }
    Sockets::closeSocket(listenSocket);
}

void ListenThread::addConnectedSocket(SOCKET s) 
{
    auto lock = Lock(connSocketsMutex);
    connSockets.push_back(s);
}

std::vector<SOCKET> ListenThread::getConnectedSockets()
{
    auto lock = Lock(connSocketsMutex, std::try_to_lock);
    if (!lock.owns_lock() || connSockets.empty()) { return std::vector<SOCKET>(); }

    std::vector<SOCKET> outSockets = connSockets;
    connSockets.clear();
    return outSockets;
}
