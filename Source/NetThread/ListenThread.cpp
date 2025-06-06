// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "ListenThread.h"
#include "StreamThread.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include <cassert>
#include <thread>
#include <chrono>

ListenThread::~ListenThread() 
{ 
    stop();
    thread.join();
}

void ListenThread::start(std::string_view port, std::string_view hostname)
{
    listenPort = port;
	selfHostname = hostname;
    assert(!listenPort.empty() && "valid port number must be provided for listen socket");
    thread = std::thread([this] { this->threadMain(); }); // create thread
}

void ListenThread::threadMain()
{
	WIN_SET_THREAD_NAME(L"Listen Thread");
    SOCKET listenSocket = INVALID_SOCKET;
    forceTerminate = !Sockets::createListenSocket(listenSocket, listenPort, selfHostname);
	
    // listener loop
    while (!forceTerminate)
    {
        SOCKET s = accept(listenSocket, nullptr, nullptr); // try to accept a connection
        if (s != INVALID_SOCKET)
		{
			ESLog::es_detail("New connection established");
			addConnectedSocket(s);
		}

		if (getNumNewlyConnected() >= 10)
		{
			// temporarily stop accepting connections if there are too many requests to connect
			ESLog::es_detail(ESLog::FormatStr() << getNumNewlyConnected() << " pending new connections, slowing down acceptance rate");
			while (getNumNewlyConnected() >= 10)
				Sockets::threadSleep(80);
		}
    }
    Sockets::closeSocket(listenSocket);
}

void ListenThread::addConnectedSocket(SOCKET s) 
{
    auto lock = Lock(connSocketsMutex);
    connSockets.push_back(s);
}

size_t ListenThread::getNumNewlyConnected()
{
	auto lock = Lock(connSocketsMutex);
	return connSockets.size();
}

std::vector<SOCKET> ListenThread::getConnectedSockets()
{
    auto lock = Lock(connSocketsMutex, std::try_to_lock);
    if (!lock.owns_lock() || connSockets.empty()) { return std::vector<SOCKET>(); }

    std::vector<SOCKET> outSockets = connSockets;
    connSockets.clear();
    return outSockets;
}
