// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "NetAgent/Agent.h"
#include "NetAgent/HttpServerUtils/HttpUtil.h"

#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <filesystem>
#include <future>


namespace HTTP
{
	// HTTP server agent
	class HttpServer : public Agent
	{
	public:
		enum class HttpMode { HTTP, HTTPS };
		enum class ServerMode { Static, Dynamic };
		HttpServer(HttpMode httpMode, ServerMode serverMode);

		void bindRequestHandler(HttpMethodType httpMethod, std::function<HttpResponse(const HttpRequest&)> handlerFunction);
		void bindRequestHandler(std::string_view filesystemWebrootPath);
		void start(std::string_view address, std::string_view port = "");
		void handleRequests();

	protected:
		HttpMode httpMode;
		ServerMode serverMode;
		std::vector<HttpHandlerBinding> handlers;
		std::list<std::future<HttpTaskResult>> futures;
		HttpFilesystem httpFilesystem{};
		static HttpTaskResult handleHttpRequest(Connection& connection, std::vector<HttpHandlerBinding>& methodHandlers);
		static HttpRequest parseHttpRequest(const std::string& request, HttpStatusCode& parserStatusOut);
		HttpResponse filesystemRequestHandler(const HttpRequest& request) const;
		HttpResponse dynamicRequestHandler(const HttpRequest& request) const;
	};
}
