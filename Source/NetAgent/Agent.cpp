// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/Agent.h"
#include "NetThread/StreamThread.h"
#include "NetThread/ListenThread.h"
#include "Sockets/Sockets.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include <algorithm>

Agent::Agent(Mode mode)
{
	Sockets::init();
	listenThread = (mode == Mode::Server) ? std::make_unique<ListenThread>() : nullptr;
}

Agent::~Agent()
{ 
	Sockets::cleanup(); 
}

Connection& Agent::connect(std::string_view hostname, std::string_view port)
{
	connections.push_back(Connection(hostname, port));
	return connections.back();
}

void Agent::listen(std::string_view port, std::string_view hostname)
{ 
	assert(isServer());
	listenThread->start(port, hostname);
}

void Agent::stopListening()
{ 
	assert(isServer());
	listenThread->stop(); 
}

Connection& Agent::getConnection(ConnectionId id)
{ 
	return connections[id]; 
}

size_t Agent::numConnections() const
{
	return connections.size();
}

bool Agent::updateConnections()
{
	assert(isServer() && "do not call update when in client mode");
	if (not isServer())
		return false;
	auto sockets = listenThread->getConnectedSockets();
	for (SOCKET socket : sockets)
	{ 
		if (connections.size() < connectionLimit)
			connections.push_back(Connection(socket));
		else
		{
			Sockets::shutdownConnection(socket, 2); // drop connections if limit is exceeded
			ESLog::es_detail("Connection limit exceeded, dropped connection");
		}
	}
	return true;
}

std::vector<Connection>& Agent::getAllConnections()
{
	return connections;
}


Connection::Connection(SOCKET connectedSocket) 
	: thread{ std::make_unique<StreamThread>(2048, 2048) }
{
	thread->start(connectedSocket);
}

Connection::Connection(std::string_view hostname, std::string_view port) 
	: thread{ std::make_unique<StreamThread>(256, 256) }
{
	thread->start(hostname, port, 5);
}

bool Connection::isConnected() const 
{ 
	return thread->isStreamConnected(); 
}

bool Connection::isFailed() const 
{ 
	return thread->isFailed(); 
}

void Connection::Close() 
{ 
	thread->stop(); 
}

bool Connection::send(std::string_view data) 
{ 
	return thread->queueSend(data); 
}

void Connection::receive(std::string& data) 
{ 
	thread->getReceiveBuffer(data); 
}

size_t Connection::getIncomingDataSize() const
{
	return thread->getReceiveDataSize();
}
