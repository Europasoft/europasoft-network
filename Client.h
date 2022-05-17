#pragma once
#include "StreamThread.h"

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
    size_t receiveBufferDataSize = 0;

public:
    Client();
    ~Client();

    bool isConnected() const { return streamThread.threadRunning; }

    // establish TCP connection (starts the client stream thread)
    void connectStream(const std::string& hostname = "127.0.0.1");

    // send data to remote host over TCP stream
    bool sendStream(const char* data = "TEST DATA", bool finalSend = false);
    
};