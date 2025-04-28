// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/Logging.h"

#include <functional>
#include <stdint.h>
#include <utility>
#include <array>

namespace HTTP
{

	HttpServer::HttpServer(HttpServer::HttpMode httpMode)
		: Agent{ Agent::Mode::Server }, httpMode{ httpMode }
	{
		// TODO: HTTPS (TLS) support
		ESLog::es_assertRuntime(httpMode != HttpMode::HTTPS, "SSL/TLS is not supported");
	}

	void HttpServer::bindRequestHandler(HttpMethodType httpMethod, std::function<HttpResponse(const HttpRequest&)> handlerFunction)
	{
		handlers.push_back(HttpHandlerBinding{ .method = httpMethod, .execute = handlerFunction });
	}

	void HttpServer::bindRequestHandler(std::string_view filesystemWebrootPath)
	{
		webroot = std::filesystem::path(filesystemWebrootPath);
		using namespace std::placeholders;
		ESLog::es_assertRuntime(std::filesystem::exists(filesystemWebrootPath), "The WebRoot path must point to a valid directory");
		std::function<HttpResponse(const HttpRequest&)> f = std::bind(&HttpServer::filesystemRequestHandler, this, std::placeholders::_1);
		bindRequestHandler(HttpMethodType::ANY_M, f);
	}

	void HttpServer::start(std::string_view address)
	{
		const auto port = (httpMode == HttpMode::HTTPS) ? "443" : "80";
		Agent::listen(port, address);
	}

	void HttpServer::handleRequests()
	{
		Agent::updateConnections();

		for (Connection& conn : Agent::getAllConnections())
		{
			if (conn.getIncomingDataSize() > 0)
			{
				//ESLog::es_traffic(std::format("Incoming {} bytes", conn.getIncomingDataSize()));
				ESLog::es_traffic((std::ostringstream() << "Incoming " << conn.getIncomingDataSize() << " bytes").str());
				futures.push_back(std::async(std::launch::async, &HttpServer::handleHttpRequest, std::ref(conn), std::ref(handlers)));
			}
		}

		auto it = futures.begin();
		while (it != futures.end())
		{
			std::future<HttpTaskResult>& future = *it;
			if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				const HttpTaskResult result = future.get();
				//ESLog::es_traffic(std::format("Request\n{}\nProcessed: '{} {}'\n", result.originalRequest, (uint32_t)result.statusCode, httpStatusCodeToString(result.statusCode)));
				ESLog::es_traffic((std::ostringstream() << "Request\n" << result.originalRequest 
								<< "\nProcessed: '" << (uint32_t)result.statusCode << " " << httpStatusCodeToString(result.statusCode) << "'").str());
				it = futures.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	HttpTaskResult HttpServer::handleHttpRequest(Connection& connection, std::vector<HttpHandlerBinding>& methodHandlers)
	{
		WIN_SET_THREAD_NAME(L"HTTP request handler");
		std::string requestString;
		connection.receive(requestString);
		if (requestString.empty())
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST, .originalRequest = requestString };

		HttpRequest request = parseHttpRequest(requestString);

		if (request.method == HttpMethodType::UNRECOGNIZED_M)
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST, .originalRequest = requestString, .logInfo = "unrecognized request method"};
		
		for (HttpHandlerBinding& handler : methodHandlers)
		{
			if (handler.method == request.method or handler.method == HttpMethodType::ANY_M)
			{
				const HttpResponse response = handler.execute(request);
				std::string responseString = response.finalizeToString();
				connection.send(responseString);
				return HttpTaskResult{ .statusCode = response.statusCode, .originalRequest = requestString, .logInfo = "handled" };
			}
		}

		return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .originalRequest = requestString };
	}

	HttpRequest HttpServer::parseHttpRequest(const std::string& request)
	{
		auto methodEnd = request.find(" ");
		auto urlEnd = request.find(" ", methodEnd + 1);
		HttpRequest req;
		req.method = httpMethodFromString(request.substr(0, methodEnd));
		req.url = request.substr(methodEnd, urlEnd - methodEnd);
		return req;
	}

	HttpResponse HttpServer::filesystemRequestHandler(const HttpRequest& request)
	{
		if (request.method != HttpMethodType::GET_M)
			return HttpResponse::errorResponse(HttpStatusCode::METHOD_NOT_ALLLOWED);

		auto fullPath = webroot / std::filesystem::path(request.url);
		fullPath = std::filesystem::weakly_canonical(fullPath);
		if (not std::filesystem::is_regular_file(fullPath))
			return HttpResponse::errorResponse(HttpStatusCode::NOT_FOUND);

		const auto content = fileToString(fullPath);
		if (content.empty())
			return HttpResponse::errorResponse(HttpStatusCode::NO_CONTENT);

		return HttpResponse
		{
			.statusCode = HttpStatusCode::OK,
			.headerFields = { "Content-Type: text/html; charset=utf-8" }, // TODO: determine content type
			.payload = content
		};
	}

}
