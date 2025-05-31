// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "Sockets/Sockets.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>

class StreamThread;
class ListenThread;

typedef size_t ConnectionId;
class NetAgentSettings;

class Connection
{
public:
	// server
    Connection(SOCKET connectedSocket, bool useEncryption, const std::shared_ptr<NetAgentSettings>& settings, ConnectionId id);
	// client
    Connection(std::string_view hostname, std::string_view port, const std::shared_ptr<NetAgentSettings>& settings, ConnectionId id);

    bool isConnected() const;
    bool isFailed() const;
    void Close();

    bool send(std::string_view data);
    void receive(std::string& data);
	size_t getIncomingDataSize() const;

	ConnectionId id = 0;

private:
    std::unique_ptr<StreamThread> thread;
};

/* CLIENT
* 1. Initialize, resolve host address
* 2. Establish connection to host
* 3. Send/receive through stream thread */

/* SERVER
* 1. Initialize, resolve own address
* 2. Create listen socket, bind socket to own address/port
* 3. Listen for connections
* 4. Commence send/receive on connected stream threads */
class Agent
{
public:
    enum class Mode { Server, Client, ServerEncrypted };
    Agent(Mode mode);
    ~Agent();

    // establish TCP connection to a remote host, returns a handle to the new connection, client mode only
	Connection& connect(std::string_view hostname, std::string_view port);

    // begin accepting connections, server mode only
    void listen(std::string_view port, std::string_view hostname);
    // stop accepting connections, server mode only
    void stopListening();

    Connection& getConnection(ConnectionId id);
    size_t numConnections() const;
	bool updateConnections();
	std::vector<Connection>& getAllConnections();
    
    bool isServer() const { return listenThread.get(); }
	void applySettings(const NetAgentSettings& settingsNew);
    
protected:
    std::vector<Connection> connections;
    std::unique_ptr<ListenThread> listenThread;
	Mode mode;
	std::shared_ptr<NetAgentSettings> settings = nullptr;
	ConnectionId connectionIdCounter = 0;
};

// settings for a network agent instance
class NetAgentSettings
{
public:
	// server: maximum number of connections that can be active at the same time
	size_t connectionsMax = 100;

	// maxmimum time to keep a connection open when no communication is happening (seconds)
	double communicationGapMaxSec = 10.0;

	// time after which to slow down a connection when no communication is happening (seconds)
	double communicationGapSlowdownDelaySec = 1.5;

	// how much to slow down the connection when communicationGapMaxSec is exceeded (milliseconds)
	double communicationGapSlowdownAmountMs = 50.0;

	// server: temporarily stop accepting connections if there are more than this amount of unhandled connection requests
	size_t concurrentConnectRequestsMax = 10;

	// server: slow down the listen thread acceptance check loop by this amount while concurrentConnectRequestsMax is exceeded (milliseconds)
	double connectRequestOverloadDelayMs = 80.0;

	// maximum time a receive operation can wait if data is not available (milliseconds)
	double socketMaxReceiveWaitMs = 10.0;

	// client: maximum acceptable time for a client connection to be established (seconds)
	double clientConnectTimeoutSec = 3.0;
};