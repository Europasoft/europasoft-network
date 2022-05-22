#include "Client.h"

Client::Client() 
{
	Sockets::init();
}
Client::~Client()
{
	Sockets::cleanup();
}

void Client::connectStream(const std::string& hostname, const std::string& port) { streamThread.start(hostname, port); }

bool Client::sendStream(const char* data, const size_t& size, bool finalSend)
{
	const auto r = streamThread.queueSend(data, size);
	if (finalSend) { streamThread.shutdownOutgoing(); } // disconnect outgoing only
	return r;
}

size_t Client::getReceiveBuffer(char* outputBuffer, const size_t& outputBufferSize)
{
	return streamThread.getReceiveBuffer(outputBuffer, outputBufferSize);
}

