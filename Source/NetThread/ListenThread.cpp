#include "ListenThread.h"
#include "StreamThread.h"
#include <cassert>

ListenThread::ListenThread(StreamThread& streamThread) : streamThreadPtr{ &streamThread } {};
ListenThread::~ListenThread() { terminateThread(); }

void ListenThread::start(const std::string& port)
{
    listenPort = port;
    assert(!listenPort.empty() && "valid port number must be provided for listen socket");
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void ListenThread::threadMain()
{
    bool terminate = false;

    SOCKET listenSocket = INVALID_SOCKET;
    // create listen socket
    if (!Sockets::listenSocket(listenSocket, listenPort)) { terminate = true; }

    // listener loop (currently only accepts the first connection request)
    while (!terminate)
    {
        if (!streamThreadPtr->isStreamConnected())
        {
            SOCKET s = accept(listenSocket, nullptr, nullptr); // try to accept a connection
            if (s == INVALID_SOCKET) { continue; }
            streamThreadPtr->start(s); // hand over the connected socket
            terminateThread(); // TODO: support multiple connections
        }
        terminate = forceTerminate || terminate;
    }
    Sockets::closeSocket(listenSocket);
    // thread terminates at this point
}
