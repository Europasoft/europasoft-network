#pragma once
#include "NetAgent/Server.h"
#include <iostream>

class ChatClient
{
#ifdef NET_SERVER_ONLY
	Server agent{};
#else
	Client agent{};
#endif

public:
	// hostname only applies for client (ignored when compiling server)
	void connect(const std::string& port, const std::string& hostname);
	bool connected() const { return agent.isConnected(); }
	bool failed() const { return agent.connectionFailed(); }
	bool sendString(const std::string& str);
	std::string receiveString(const uint32_t& retryMaxAttempts = 100);
};

