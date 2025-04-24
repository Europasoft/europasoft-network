#include "NetAgent/Agent.h"
#include "Sockets/Sockets.h"
#include <iostream>
#include <string>
#include <vector>

void askFor(std::string& v, const std::string& def, const std::string& prompt)
{
	std::cout << "\n" << prompt;
	std::getline(std::cin, v);
	if (v.empty() || (v.find_first_not_of(" ") == v.npos)) { v = def; }
}

// Runs as either a chat server or chat client
// Test by first launching one process as server, and then another as client
// Messages can be sent in either direction between the peers
int tcpChatExample()
{
	std::string serverOrClient;
	askFor(serverOrClient, "s", "Enter s or c to launch either server or client: ");
	Agent::Mode mode = serverOrClient == "s" ? Agent::Mode::Server : Agent::Mode::Client;

	if (mode == Agent::Mode::Server)
		std::cout << "\nLaunching as Server...";
	else
		std::cout << "\nLaunching as Client...";

	// create the network agent, either in server or client mode
	Agent agent{ mode };

	std::string hostname, port;

	if (agent.isServer())
	{
		system("title SERVER"); // setting the window title like this only works on windows
		askFor(port, "5001", "Enter port (leave blank for default): ");
		std::cout << "\nWaiting for client connection...";
		agent.listen(port, "127.0.0.1");
		while (!agent.numConnections())
			Sockets::threadSleep(10);
	}
	else
	{
		system("title CLIENT");
		askFor(hostname, "localhost", "Enter hostname (leave blank for local): ");
		askFor(port, "5001", "Enter port (leave blank for default): ");
		std::cout << "\nConnecting to server...";
		auto i = agent.connect(hostname, port);
		while (not agent.getConnection(i).isConnected() and not agent.getConnection(i).isFailed())
			Sockets::threadSleep(10);
		if (agent.getConnection(i).isFailed())
		{
			std::cout << "\nConnection failed";
			std::cin.ignore();
			return 1;
		}
		std::cout << "\n\nConnection established";
	}

	std::cout << "\nLeave message blank and press enter to receive, or enter a message to send\n";
	for (;;)
	{
		auto numConn = agent.numConnections();
		// receive
		std::string instr;
		for (size_t i = 0; i < numConn; i++)
		{
			std::string rstr;
			agent.getConnection(i).receive(rstr);
			if (!rstr.empty())
			{
				if (agent.isServer()) { instr += "\nClient " + std::to_string(i + 1); }
				else { instr += "\nServer"; }
				instr += +": " + rstr;
			}
		}
		if (!instr.empty()) { std::cout << instr << "\n"; }

		// send
		std::string msg{};
		std::getline(std::cin, msg);
		if (!msg.empty())
		{
			numConn = agent.numConnections();
			for (size_t i = 0; i < numConn; i++)
			{
				bool sendSuccess = false;
				while (!sendSuccess) { sendSuccess = agent.getConnection(i).send(msg); }
			}
		}

	}
}
