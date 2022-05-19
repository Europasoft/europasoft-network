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

bool Client::sendStream(const char* data, bool finalSend)
{
	const auto r = streamThread.queueSend(*data, strlen(data));
	if (finalSend) { streamThread.shutdownOutgoing(); } // disconnect outgoing only
	return r;
}

bool Client::getReceiveBuffer(size_t& dataSizeOut, char& outputBuffer, const size_t& outputBufferSize)
{
	const auto datasize = streamThread.getReceiveBuffer(outputBuffer, outputBufferSize);
	dataSizeOut = datasize;
	return (datasize > 0);
}

bool Client::getReceiveBuffer_String(std::string& str, const size_t& maxLength)
{
	auto bs = maxLength + 128;
	char* b = new char[bs];
	size_t ds = 0;
	if (!getReceiveBuffer(ds, *b, bs)) { delete[] b; return false; }

	str = std::string(&b[0], &b[0] + min(ds, bs));
	delete[] b;
	return true;
}

