#include "Client.h"
#include <iostream>
Client::Client() 
{
	Sockets::init();
}
Client::~Client()
{
	Sockets::cleanup();
}

void Client::connectStream(const std::string& hostname, const std::string& port) { streamThread.start(hostname, port); }

bool Client::sendStream(const char* data, const size_t& size)
{
	return streamThread.queueSend(data, size);
}

bool Client::sendStream(const std::string& str)
{
	return sendStream(str.c_str(), str.size());
}

size_t Client::getReceiveBuffer(char* outputBuffer, const size_t& outputBufferSize)
{
	return streamThread.getReceiveBuffer(outputBuffer, outputBufferSize);
}

std::string Client::getReceiveBuffer(const size_t& maxLength)
{
	auto* b = new char[maxLength];
	size_t bSize = streamThread.getReceiveBuffer(b, maxLength);
	const auto str = std::string(b, bSize);
	std::cout << "\nreceived " << bSize;
	delete[] b;
	if (bSize <= 0) { return std::string(); }
	return str;
}


