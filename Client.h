#pragma once
#include "SocketThread.h"

class Client 
{
    using MutexSocket = Sockets::MutexSocket;
    using Lock = MutexSocket::Lock;

    SocketStreamThread socketThread{};
    MutexSocket clientSocket;
    
public:
    Client() { Sockets::init(); }
    ~Client() { Sockets::cleanup(); }

protected:
    // resolve hostname and establish TCP connection
    bool connectStream(const std::string& host = "127.0.0.1")
    {
        addrinfo* hostAddr = nullptr;
        if (!Sockets::resolveHostname(host, hostAddr)) { return false; }
        SOCKET s = 0;
        return Sockets::connectSocket(hostAddr, s);
        Lock lock{};
        clientSocket.set(s);
        freeaddrinfo(hostAddr);

        socketThread.start(&clientSocket);
    }

    // send data to remote host over TCP stream
    bool sendStream(const char* data = "TEST DATA", bool sendAndShutdown = false)
    {
        if (!socketThread.queueSend(*data, strlen(data))) { return false; }
        Lock lock;
        if (sendAndShutdown) { Sockets::shutdownConnection(clientSocket.get(lock), 1); } // shutdown outgoing only
    }
    

    

};
