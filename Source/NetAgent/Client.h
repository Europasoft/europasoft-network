#pragma once
#include "Sockets/StreamThread.h"

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

    // establish TCP connection (starts the client stream thread)
    virtual void connectStream(const std::string& hostname = std::string(), const std::string& port = "27015");

    bool isConnected() const { return streamThread.isThreadRunning(); }

    // send data to remote host over TCP stream
    bool sendStream(const char* data = "nodata", bool finalSend = false);

    /*  returns pointer to the main-thread copy of the receive buffer,
    *   if (update==true) thread-safely copies stream-thread buffer to main-thread buffer */
    bool getReceiveBuffer(size_t& dataSizeOut, char& outputBuffer, const size_t& outputBufferSize);

    bool getReceiveBuffer_String(std::string& str, const size_t& maxLength = 256);
    
    
    
};