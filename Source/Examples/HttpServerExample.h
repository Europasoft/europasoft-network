// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpServerUtils/Logging.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>

// This function demonstrates a minimal custom request handler, it is used as a callback bound to the server
HTTP::HttpResponse helloHandler(const HTTP::HttpRequest& request)
{
	if (request.url != "/hello")
		// Only process requests with the URL "hello", otherwise reject it (other handlers may handle it)
		return HTTP::HttpResponse::unhandledResponse();

	HTTP::HttpResponse response
	{
		.statusCode = HTTP::HttpStatusCode::OK,
		.payload = "Hello from the server!"
	};
	response.addHeaderField("Content-Type", "text/plain; charset=utf-8");

	ESLog::es_info("Serving hello message to client");

	return response;

}

// Can serve files (the files must be in the webroot directory specified when calling the function)
// Also demonstrates how to set up custom API endpoints to return arbitrary data
// Test by visiting "127.0.0.1" in a web browser (or use "127.0.0.1/hello" to send a request to the API)
int httpServerExample(std::string_view localWebrootPath)
{
	using namespace HTTP;

	// Set logging settings for the HTTP server
	ESLog::setGlobalLogSettings(
		ESLog::GlobalLogSettings
		{
			.logLevel = ESLog::Lvl::ES_DETAIL,
			.enableLogToFile = false,
			.enableLogToOutput = true,
			.disableAllLogging = false
		});

	// Using unencrypted HTTP for the example here, to avoid needing to get a certificate just to run the demo.
	// When the server is set to static mode it behaves like a classic webserver.
	// However, if dynamic mode is selected instead, the site will be loaded as a Single Page Application (SPA).  
	// SPA pages are never fully reloaded, and content is updated directly in the user's browser, through bootstrapping code which runs clientside.
	HttpServer server = { HttpServer::HttpMode::HTTP, HttpServer::ServerMode::Static };

	// Bind the "hello handler" to show how custom API request logic can be added
	server.bindRequestHandler(HTTP::HttpMethodType::GET_M, helloHandler);
	// Bind the filesytem handler, this will serve any files present in the specified webroot directory (like index.html)
	server.bindRequestHandler(localWebrootPath);

	// Start the server. The port for unencrypted HTTP is set to 80 by default (that is standard).
	// This can be customized, for example to use port 1024: "127.0.0.1:1024", or to specify a different network interface
	// 127.0.0.1 (localhost) is good for the demo, as it does not expose the server to the internet. Only to users on the same machine.
	server.start("127.0.0.1", "80");

	// Finally, to keep the server running, handleRequests() must be called repeatedly. 
	// Internally it uses subthreads to manage connections, but this allows checking things like connection count, without synchronization concerns.
	// Here the server state is updated in an infinite loop
	for (;;)
	{
		server.handleRequests();
		// Idle the loop for a short moment, to allow processing time for other programs and avoid constant CPU usage
		std::this_thread::sleep_for(std::chrono::duration<long long, std::milli>(30));
	}
}