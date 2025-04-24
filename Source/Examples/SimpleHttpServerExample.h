// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/Agent.h"
#include <iostream>
#include <string>
#include <vector>

// Responds with a simple HTTP message to the first request received
// Test by visiting "127.0.0.1" in a web browser
int simpleHttpServerExample()
{
	Agent agent{ Agent::Mode::Server };
	agent.listen("80", "127.0.0.1");
	std::cout << "\Listening for browser connections";
	while (agent.numConnections() == 0) { Sockets::threadSleep(300); };
	std::cout << "\nClient connected";

	for (;;)
	{
		std::string incoming;
		agent.getConnection(0).receive(incoming);
		if (not incoming.empty())
		{
			std::cout << "\nReceived message from client:\n" << incoming;
			std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 20\r\nContent-Type: text/plain; charset=utf-8\r\n\r\nResponse from server";
			agent.getConnection(0).send(response);
			Sockets::threadSleep(1000);
			break;
		}

	}
	return 0;
}