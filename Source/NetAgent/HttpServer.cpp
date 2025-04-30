// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpServerUtils/Logging.h"

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
		using namespace std::placeholders;
		ESLog::es_assertRuntime(std::filesystem::exists(filesystemWebrootPath), "The WebRoot path must point to a valid directory");
		httpFilesystem.updateFullRefresh(filesystemWebrootPath);
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
				ESLog::es_traffic(ESLog::FormatStr() << "Incoming " << conn.getIncomingDataSize() << " bytes");
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
				const std::string status = ESLog::FormatStr() << (uint32_t)result.statusCode << " " << httpStatusCodeToString(result.statusCode);
				ESLog::es_traffic(ESLog::FormatStr() << "Processed request\n{\n\t" << result.request.toShortString() << "\n}\n" << status << "\n");
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
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST };

		HttpStatusCode parserStatus = HttpStatusCode::SRV_ERROR;
		HttpRequest request = parseHttpRequest(requestString, parserStatus);

		// return early for issues found during parsing
		if (httpStatusCodeIsError(parserStatus) or request.method == HttpMethodType::UNRECOGNIZED_M)
		{
			if (parserStatus == HttpStatusCode::URI_TOO_LONG or 
				parserStatus == HttpStatusCode::PAYLOAD_TOO_LARGE or 
				parserStatus == HttpStatusCode::NO_REQUEST_LENGTH)
				return HttpTaskResult{ .statusCode = parserStatus, .request = request };
			if (request.method == HttpMethodType::UNRECOGNIZED_M)
				return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .request = request };
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST, .request = request };
		}
		
		for (HttpHandlerBinding& handler : methodHandlers)
		{
			if (handler.method == request.method or handler.method == HttpMethodType::ANY_M)
			{
				const HttpResponse response = handler.execute(request);
				std::string responseString = response.finalizeToString();
				connection.send(responseString);
				return HttpTaskResult{ .statusCode = response.statusCode, .request = request };
			}
		}

		return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .request = request };
	}

	HttpRequest HttpServer::parseHttpRequest(const std::string& request, HttpStatusCode& parserStatusOut)
	{
		auto methodEnd = request.find(" ");
		auto urlEnd = request.find(" ", methodEnd + 1);

		// catch HTTP methods that are longer than the longest supported
		if (methodEnd > 7)
		{
			parserStatusOut = HttpStatusCode::METHOD_NOT_ALLLOWED;
			return HttpRequest{};
		}
		// catch URIs that are unacceptably long
		if (urlEnd - methodEnd > 9000)
		{
			parserStatusOut = HttpStatusCode::URI_TOO_LONG;
			return HttpRequest{};
		}
		// TODO: check payload length, header fields, and request length value in request

		HttpRequest req;
		req.method = httpMethodFromString(request.substr(0, methodEnd));
		req.url = request.substr(methodEnd, urlEnd - methodEnd);
		parserStatusOut = HttpStatusCode::OK;
		return req;
		
	}

	HttpResponse HttpServer::filesystemRequestHandler(const HttpRequest& request) const
	{
		if (request.method != HttpMethodType::GET_M)
			return HttpResponse::errorResponse(HttpStatusCode::METHOD_NOT_ALLLOWED);

		auto url = request.url;
		//auto url = "../noaccess.txt";//request.url

		std::erase(url, ' ');
		const auto fileId = httpFilesystem.findFile(url);
		if (not fileId)
			return HttpResponse::errorResponse(HttpStatusCode::NOT_FOUND);

		std::string content;
		if (not httpFilesystem.getFileAsString(fileId, content))
			return HttpResponse::errorResponse(HttpStatusCode::SRV_ERROR);
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
