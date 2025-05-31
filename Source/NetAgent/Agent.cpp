// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/Agent.h"
#include "NetThread/StreamThread.h"
#include "NetThread/ListenThread.h"
#include "Sockets/Sockets.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include <algorithm>

Agent::Agent(Mode mode)
	: mode{ mode }
{
	Sockets::init();
	listenThread = (mode != Mode::Client) ? std::make_unique<ListenThread>() : nullptr;
}

Agent::~Agent()
{ 
	Sockets::cleanup(); 
}

Connection& Agent::connect(std::string_view hostname, std::string_view port)
{
	// apply default server settings if not set
	if (not settings)
		applySettings(NetAgentSettings());
	// client mode only
	connections.push_back(Connection(hostname, port, settings, connectionIdCounter++));
	return connections.back();
}

void Agent::listen(std::string_view port, std::string_view hostname)
{ 
	assert(isServer());
	// apply default server settings if not set
	if (not settings)
		applySettings(NetAgentSettings());
	// start listen thread to begin accepting connections
	listenThread->start(port, hostname);
}

void Agent::stopListening()
{ 
	assert(isServer());
	listenThread->stop(); 
}

Connection& Agent::getConnection(ConnectionId id)
{ 
	for (auto& conn : connections)
	{
		if (conn.id == id)
			return conn;
	}
}

size_t Agent::numConnections() const
{
	return connections.size();
}

bool Agent::updateConnections()
{
	assert(isServer() && "do not call updateConnections when in client mode");
	if (not isServer())
		return false;
	auto sockets = listenThread->getConnectedSockets();
	for (SOCKET socket : sockets)
	{ 
		if (connections.size() < settings->connectionsMax)
			connections.push_back(Connection(socket, (mode == Agent::Mode::ServerEncrypted), settings, connectionIdCounter++));
		else
		{
			Sockets::shutdownConnection(socket, 2); // drop connections if limit is exceeded
			ESLog::es_detail("Connection limit exceeded, dropped connection");
		}
	}
	for (auto it = connections.begin(); it != connections.end();)
	{
		if (it->isFailed() || !it->isConnected()) 
		{
			ESLog::es_detail(ESLog::FormatStr() << "Connection " << it->id << " removed, " << connections.size() << " active");
			it = connections.erase(it);
		}
		else 
		{
			++it;
		}
	}
	
	return true;
}

std::vector<Connection>& Agent::getAllConnections()
{
	return connections;
}

void Agent::applySettings(const NetAgentSettings& settingsNew)
{
	settings = std::make_shared<NetAgentSettings>(settingsNew);
}

Connection::Connection(SOCKET connectedSocket, bool useEncryption, const std::shared_ptr<NetAgentSettings>& settings, ConnectionId id)
	: thread{ std::make_unique<StreamThread>(2048, 2048, 
		useEncryption ? StreamEncryptionMode::Encrypted : StreamEncryptionMode::NoEncryption) },
	id{ id }
{
	thread->updateSettings(settings);
	thread->start(connectedSocket);
}

Connection::Connection(std::string_view hostname, std::string_view port, const std::shared_ptr<NetAgentSettings>& settings, ConnectionId id)
	: thread{ std::make_unique<StreamThread>(256, 256, StreamEncryptionMode::NoEncryption) },
	id{ id }
{
	thread->updateSettings(settings);
	thread->start(hostname, port);
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
	if (data.size() > 0)
		ESLog::es_detail(ESLog::FormatStr() << "Connection " << id << " received data");
}

size_t Connection::getIncomingDataSize() const
{
	return thread->getReceiveDataSize();
}
