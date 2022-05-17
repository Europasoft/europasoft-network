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

