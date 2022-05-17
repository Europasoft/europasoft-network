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

void Client::connectStream(const std::string& hostname)
{
	streamThread.start(&streamSocket, hostname);
}

bool Client::sendStream(const char* data, bool finalSend)
{
	if (!streamThread.queueSend(*data, strlen(data))) { return false; }
	if (finalSend) { Lock lock; Sockets::shutdownConnection(streamSocket.get(lock), 1); } // shutdown outgoing only
}

