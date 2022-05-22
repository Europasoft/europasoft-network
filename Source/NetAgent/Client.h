#pragma once
#include "NetThread/StreamThread.h"

/* CLIENT EXECUTION PATH 
* 1. Initialize, resolve host address/port (DNS query)
* 2. Connect to host, start stream thread
* 3. Send and receive through stream thread
* 4. Keep sending as long as thread is running
* 5. Terminate thread on connection closed */

class Client 
{
protected:
    StreamThread streamThread{};
public:
    Client();
    ~Client();

    void noRecv() { streamThread.skipRecv = true; }

    // establish TCP connection (starts the client stream thread)
    virtual void connectStream(const std::string& hostname = std::string(), const std::string& port = "27015");

    bool isConnected() const { return streamThread.isThreadRunning(); }

    // send data to remote host over TCP stream
    bool sendStream(const char* data, const size_t& size, bool finalSend = false);

    //  copies data from receive buffer to a main-thread buffer
    size_t getReceiveBuffer(char* outputBuffer, const size_t& outputBufferSize);

};