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

// Runs as either a chat server or chat client, depending on if the preprocessor flag NET_SERVER_ONLY is defined
// Test by first launching one process as server, and then another as client (i.e. without the flag defined)
// Messages can be sent in either direction between the peers
int tcpChatExample()
{
#ifdef NET_SERVER_ONLY
	Agent agent{ Agent::Mode::Server };
#else
	Agent agent{ Agent::Mode::Client };
#endif

	std::string hostname, port;

#ifdef NET_SERVER_ONLY
	system("title SERVER");
	askFor(port, "5001", "Enter port: ");
	std::cout << "\nWaiting for remote...";
	agent.listen(port, "127.0.0.1");
	while (!agent.numConnections());

#else
	system("title CLIENT");
	askFor(hostname, "localhost", "Enter hostname: ");
	askFor(port, "5001", "Enter port: ");
	std::cout << "\nConnecting...";
	auto i = agent.connect(hostname, port);
	while (!agent.getConnection(i).isConnected() && !agent.getConnection(i).isFailed());
	if (agent.getConnection(i).isFailed())
	{
		std::cout << "\nConnection failed";
		std::cin.ignore();
		return 1;
	}
	std::cout << " Connection established";

#endif

	std::cout << "\nLeave message blank and press enter to receive\n";
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
