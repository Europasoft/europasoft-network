#pragma once
#include "NetAgent/Client.h"
#include "NetThread/ListenThread.h"

/* SERVER EXECUTION PATH
* 1. Initialize, resolve own address/port
* 2. Create listen socket, bind socket to own address/port
* 3. Listen for connection
* 4. On connection established, run inherited client code */

class Server : public Client
{
protected:
	ListenThread listenThread{ streamThread };
public:
	Server() = default;
	void connectStream() = delete; // disable calls to client-specific function

	// begin accepting connections
	void listenStart(const std::string& bindPort = "27015") { listenThread.start(bindPort); }

};
