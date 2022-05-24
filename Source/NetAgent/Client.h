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

    // establish TCP connection to a remote host, starts stream thread in client mode
    void connectStream(const std::string& hostname = std::string(), const std::string& port = "27015");

    // returns current state of the stream thread
    bool isConnected() const { return streamThread.isStreamConnected(); }
    bool connectionFailed() const { return streamThread.isFailed(); }

    // send data to remote host over TCP stream
    bool sendStream(const char* data, const size_t& size);
    bool sendStream(const std::string& str);

    //  copies data from receive buffer to a main-thread buffer
    size_t getReceiveBuffer(char* outputBuffer, const size_t& outputBufferSize);
    std::string getReceiveBuffer(const size_t& maxLength = 256);
};