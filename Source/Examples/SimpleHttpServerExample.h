// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/Agent.h"
#include <iostream>
#include <string>
#include <vector>

#include <fstream>
#include <sstream>

std::string fileToString(const std::string& filename)
{
	std::ifstream t(filename);
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	std::string buffer(size, ' ');
	t.seekg(0);
	t.read(&buffer[0], size);
	return buffer;
}

std::string fileNotFoundHttpResponse()
{
	return "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/plain; charset=utf-8\r\n\r\nNot found";
}

std::string htmlFileHttpResponse(const std::string& filename)
{
	auto buffer = fileToString(filename);
	return "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(buffer.size()) + "\r\nContent-Type: text/html; charset=utf-8\r\n\r\n" + buffer;
}

std::string cssFileHttpResponse(const std::string& filename)
{
	auto buffer = fileToString(filename);
	return "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(buffer.size()) + "\r\nContent-Type: text/css; charset=utf-8\r\n\r\n" + buffer;
}

// Responds with a simple HTTP message to the first request received
// Test by visiting "127.0.0.1" in a web browser
int simpleHttpServerExample(const std::string& htmlFilePath, const std::string& cssFilePath)
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

			std::string response;
			if (incoming.find("GET / ") != std::string::npos)
			{
				response = htmlFileHttpResponse(htmlFilePath);
			}
			else if (incoming.find("favicon") != std::string::npos)
			{
				response = fileNotFoundHttpResponse();
			}
			else if (incoming.find(".css") != std::string::npos)
			{
				response = cssFileHttpResponse(cssFilePath);
			}
			
			std::cout << "\nReplied:\n" << response << "\n";
			agent.getConnection(0).send(response);
			
			//break;
		}
		Sockets::threadSleep(30);

	}
	return 0;
}