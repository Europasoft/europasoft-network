#pragma once
#include "SocketThread.h"

class Client 
{
    using MutexSocket = Sockets::MutexSocket;
    using Lock = MutexSocket::Lock;

public:
    Client(const size_t& bufferSize = 512) { Sockets::init(); }
    ~Client() { Sockets::cleanup(); }


protected:
    // resolve hostname and establish TCP connection
    bool connectStream(const std::string& host = "127.0.0.1")
    {
        addrinfo* hostAddr = nullptr;
        if (!Sockets::resolveHostname(host, hostAddr)) { return false; }
        SOCKET stream_socket = 0;
        return connected = Sockets::connectSocket(hostAddr, stream_socket);
        Lock lock{};
        msocket.set(stream_socket);
        freeaddrinfo(hostAddr);
    }
    // send data to remote host over TCP stream
    bool sendStream(const char* data = "TEST DATA", bool sendAndShutdown = false)
    {
        if (!connected || !Sockets::sendData(socket0, data)) { return false; }
        if (sendAndShutdown) { Sockets::shutdownConnection(socket0, 1); } // shutdown outgoing only
    }
    

    

};
