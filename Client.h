#pragma once
#include "SocketThread.h"

/* CLIENT EXECUTION PATH */
// 1. Initialize, resolve host address/port (DNS query)
// 2. Connect to host, start stream thread
// 3. Send and receive through stream thread
// 4. Keep sending as long as thread is running
// 5. Terminate thread on connection closed

class Client 
{
    using MutexSocket = Sockets::MutexSocket;
    using Lock = MutexSocket::Lock;

    MutexSocket streamSocket;
    SocketStreamThread streamThread{};
    // for async access to data from the actual receive buffer (which may be in-use by stream thread)
    char* receiveBuffer = nullptr;

public:
    Client();
    ~Client();

    // resolve hostname and establish TCP connection
    bool connectStream(const std::string& host = "127.0.0.1")
    {
        addrinfo* hostAddr = nullptr;
        if (!Sockets::resolveHostname(host, hostAddr)) { return false; }
        SOCKET s = 0;
        return Sockets::connectSocket(hostAddr, s);
        Lock lock{};
        streamSocket.set(s);
        freeaddrinfo(hostAddr);

        streamThread.start(&streamSocket);
    }

    // send data to remote host over TCP stream
    bool sendStream(const char* data = "TEST DATA", bool finalSend = false)
    {
        if (!streamThread.queueSend(*data, strlen(data))) { return false; }
        Lock lock;
        if (finalSend) { Sockets::shutdownConnection(streamSocket.get(lock), 1); } // shutdown outgoing only
    }
    
};