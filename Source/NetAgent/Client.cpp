#include "Client.h"

Client::Client() 
{
	Sockets::init();
	receiveBuffer = new char[streamThread.recMaxSize];
}
Client::~Client()
{
	Sockets::cleanup();
	delete[] receiveBuffer;
}

void Client::connectStream(const std::string& hostname) { streamThread.start(&streamSocket, hostname); }

bool Client::sendStream(const char* data, bool finalSend)
{
	const auto r = streamThread.queueSend(*data, strlen(data));
	if (finalSend) { Lock lock; Sockets::shutdownConnection(streamSocket.get(lock), 1); } // shutdown outgoing only
	return r;
}

char* Client::getReceiveBuffer(size_t& dataSizeOut, bool update) 
{
	if (!update) 
	{
		dataSizeOut = receiveBufferDataSize;
		return receiveBuffer;
	}
	const auto datasize = streamThread.getReceiveBuffer(*receiveBuffer, streamThread.recMaxSize);
	dataSizeOut = receiveBufferDataSize = datasize;
	return receiveBuffer;
}

