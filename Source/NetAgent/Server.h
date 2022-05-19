#pragma once
#include "NetAgent/Client.h"
#include "Sockets/ListenThread.h"

/* SERVER EXECUTION PATH
* 1. Initialize, resolve own address/port
* 2. Create listen socket, bind socket to own address/port
* 3. Listen for connection
* 4. On connection established, run inherited client code */

class Server : public Client
{
protected:
	ListenThread listenThread{};
public:
	Server();
	~Server();

	// begin accepting connections
	void listenStart(const std::string& bindPort = "27015") { listenThread.start(bindPort); }
	// (may be called repeatedly) check whether client has connected, and if so, start the stream thread
	bool checkConnection();

	virtual void connectStream(const std::string& hostname, const std::string& port) override { return; }
};
