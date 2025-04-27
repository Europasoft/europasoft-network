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

class Connection
{
public:
    Connection(SOCKET connectedSocket); // server
    Connection(std::string_view hostname, std::string_view port); // client

    bool isConnected() const;
    bool isFailed() const;
    void Close();

    bool send(std::string_view data);
    void receive(std::string& data);
	size_t getIncomingDataSize() const;

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
    enum class Mode { Server, Client };
    Agent(Mode mode);
    ~Agent();

    // establish TCP connection to a remote host, returns a handle to the new connection
	Connection& connect(std::string_view hostname, std::string_view port);

    // begin accepting connections, server mode only
    void listen(std::string_view port, std::string_view hostname = std::string());
    // stop accepting connections, server mode only
    void stopListening();

    Connection& getConnection(ConnectionId id);
    size_t numConnections() const;
	bool updateConnections();
	std::vector<Connection>& getAllConnections();
    
    bool isServer() const { return listenThread.get(); }

	size_t connectionLimit = 100;
    
protected:
    std::vector<Connection> connections;
    std::unique_ptr<ListenThread> listenThread;
    
};