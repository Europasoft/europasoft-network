// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "StreamThread.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include <cassert>
#include <limits.h>
#include <cstring>

StreamThread::StreamThread(size_t sendBufferSize, size_t receiveBufferSize)
    : sendBuffer{ sendBufferSize }, recvBuffer{ receiveBufferSize } {}

StreamThread::~StreamThread() 
{ 
    stop();
    thread.join();
}

void StreamThread::start(std::string_view hostname_, std::string_view port_, size_t connectTimeout_)
{
    hostname = hostname_;
    port = port_;
    connectTimeout = connectTimeout_;
    start(INVALID_SOCKET);
}

void StreamThread::start(SOCKET socket_)
{
    if (streamConnected) { return; }
    
    if (socket_ != INVALID_SOCKET)
    {
        // assumes socket is already connected (server mode)
        socket.set(socket_);
        streamConnected = true;
    }
    // start thread
    thread = std::thread([this] { this->threadMain(); }); 
}

void StreamThread::threadMain()
{
	WIN_SET_THREAD_NAME(L"Stream Thread");
    bool terminate = false;

    // resolve hostname and connect (client mode only)
    if (!streamConnected)
    {
        Timer t;
        t.start();
        SOCKET s;
        while (!t.checkTimeout(connectTimeout))
        {
            if (Sockets::setupStream(hostname, port, s))
            {
                socket.set(s);
                streamConnected = true;
                break;
            }
        }
        if (!streamConnected) 
        { 
            // timeout
            terminate = true;
            connectionFailure = true;
        }
    }

	{
		Lock socketLock;
		if (not Sockets::setReceiveTimeout(socket.get(socketLock), 10000))
			terminate = true;
	}
	lastComTimer.start();

    // thread main loop
    while (!terminate)
    {
        // send
        if (sendBuffer.getDataSize() > 0)
        {
            Lock socketLock, bufferLock;
            SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of block
            if (Sockets::sendData(s, sendBuffer.getBuffer(bufferLock), sendBuffer.getDataSize()))
            { 
                sendBuffer.setDataSize(0); // data sent, mark empty
				lastComTimer.start();
            }
        }

        // receive
        if (recvBuffer.getDataSize() == 0)
        {
            Lock socketLock;
            SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of scope
            auto recvSize = Sockets::getReceiveSize(s);
            if (recvSize > 0)
            {
				if (recvSize > 5e7)
				{
					terminate = true; // end the connection if the network buffer grows too large
					ESLog::es_detail("Connection thread terminating: received data too large");
				}
				else
				{
					Lock bufferLock;
					auto* buffer = recvBuffer.getBuffer(bufferLock);
					if (recvBuffer.reserve(recvSize, bufferLock))
					{
						recvSize = Sockets::receiveData(s, buffer, recvSize);
						if (recvSize > 0)
						{
							recvBuffer.setDataSize(recvSize);
							lastComTimer.start();
						}
						else if (recvSize < 0)
						{
							terminate = true;
							ESLog::es_detail("Connection thread terminating: receive failure");
						}
					}
					else
					{
						terminate = true;
						ESLog::es_error("Connection thread terminating: receive buffer allocation failed");
					}
				}
            }
        }
		const double delta = lastComTimer.getElapsed();
		if (delta > 10.f)
		{
			terminate = true;
			ESLog::es_detail("Connection thread terminating: comms delta timeout");
		}
		else if (delta > 1.5f)
		{
			Sockets::threadSleep(50); // idle the thread if no communication has happened in a while
		}
        
        terminate = forceTerminate || terminate;
    }
    streamConnected = false;
}

bool StreamThread::queueSend(std::string_view data)
{
    assert(streamConnected && data.size() <= sendBuffer.getBufMax() && "cannot send data");
    if (!streamConnected || !data.size() || sendBuffer.getDataSize() > 0 || forceTerminate) { return false; }

    Lock lock;
    sendBuffer.getBuffer(lock);
    return sendBuffer.copyFrom(data.data(), data.size(), lock);
}

void StreamThread::getReceiveBuffer(std::string& data) 
{
    if (recvBuffer.getDataSize() <= 0) { return; }
    Lock lock;
    data = std::string(recvBuffer.getBuffer(lock), recvBuffer.getDataSize());
    recvBuffer.setDataSize(0);
}

size_t StreamThread::getReceiveDataSize() const
{
	return recvBuffer.getDataSize();
}

size_t StreamThread::getReceiveBuffer(char* dstBuffer, const size_t& dstBufferSize)
{
    auto size = recvBuffer.getDataSize();
    if (size <= 0) { return 0; }
    assert(dstBufferSize >= size && "destination buffer too small, data will be lost");
    Lock bl;
    memcpy(dstBuffer, recvBuffer.getBuffer(bl), size);
    recvBuffer.setDataSize(0); // mark as empty ("was read")
    return size;
}
