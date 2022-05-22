#include "Server.h"

Server::Server() {};
Server::~Server() {};

bool Server::checkConnection()
{
	if (isConnected()) { return true; }
	auto s = listenThread.getConnectionSocket(); // handover of connected socket
	if (s == INVALID_SOCKET) { return false; }
	streamThread.start(s); // start server stream thread for the connected socket
	return true;
}

