// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.

// The example functions are defined in these files
#include "Examples/TcpChatExample.h"
#include "Examples/SimpleHttpServerExample.h"

#include "NetAgent/HttpServer.h"
#include "NetAgent/Logging.h"

#include <iostream>
#include <string>
#include <vector>

HTTP::HttpResponse demoHandler2(const HTTP::HttpRequest&)
{
	return HTTP::HttpResponse
	{
		.statusCode = HTTP::HttpStatusCode::OK,
		.payload = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(6) + "\r\nContent-Type: text/html; charset=utf-8\r\n\r\nhello!"
	};
}

HTTP::HttpResponse demoHandler(const HTTP::HttpRequest&)
{
	HTTP::HttpResponse response
		{
			.statusCode = HTTP::HttpStatusCode::OK,
			.payload = "hello!"
		};
	response.addHeaderField("Content-Type", "text/html; charset=utf-8");
	return response;
	
}

int main()
{
	// Run an example here, inspect the functions to see how they were implemented
	
	//return tcpChatExample();

	//return simpleHttpServerExample("C:/Users/RPG/source/repos/net-rpg/test.html", "C:/Users/RPG/source/repos/net-rpg/test.css");

	using namespace HTTP;

	ESLog::setGlobalLogSettings(
		ESLog::GlobalLogSettings
		{
			.logLevel = ESLog::Lvl::ES_TRAFFIC,
			.enableLogToFile = false,
			.enableLogToOutput = true,
		});
	

	HttpServer srv{ HttpServer::HttpMode::HTTP };
	//srv.bindRequestHandler(HttpMethodType::GET_M, &demoHandler);
	srv.bindRequestHandler("C:/Users/RPG/source/repos/net-rpg");
	srv.start("");
	for (;;)
	{
		srv.handleRequests();
		Sockets::threadSleep(30);
	}
}