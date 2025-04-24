// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/Agent.h"
#include "NetThread/StreamThread.h"
#include "NetThread/ListenThread.h"
#include "Sockets/Sockets.h"

Agent::Agent(Mode mode)
{
	Sockets::init();
	listenThread = (mode == Mode::Server) ? std::make_unique<ListenThread>() : nullptr;
}

Agent::~Agent()
{ 
	Sockets::cleanup(); 
}

size_t Agent::connect(std::string_view hostname, std::string_view port)
{
	connections.push_back(Connection(hostname, port));
	return connections.size() - 1;
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

Connection& Agent::getConnection(size_t i) 
{ 
	updateConnections();
	return connections[i]; 
}

size_t Agent::numConnections()
{
	updateConnections();
	return connections.size();
}

void Agent::updateConnections()
{
	if (!isServer()) { return; }
	auto sockets = listenThread->getConnectedSockets();
	for (auto s : sockets)
	{ 
		if (connections.size() < connectionLimit)
			connections.push_back(Connection(s));
		else
			Sockets::shutdownConnection(s, 2); // drop connections if limit is exceeded
	}
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






