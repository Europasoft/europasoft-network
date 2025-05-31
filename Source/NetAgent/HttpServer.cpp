// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpServerUtils/DynamicPages.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include "NetThread/NetThreadSync.h"

#include <functional>
#include <stdint.h>
#include <utility>
#include <array>

namespace HTTP
{
	constexpr auto ES_ENABLE_HTTPSRV_THREADING = false;

	HttpServer::HttpServer(HttpServer::HttpMode httpMode, ServerMode serverMode)
		: Agent{ (httpMode == HttpServer::HttpMode::HTTP) ? Agent::Mode::Server : Agent::Mode::ServerEncrypted }, 
		httpMode{ httpMode }, serverMode{ serverMode }
	{
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

	void HttpServer::applySettings(const NetAgentSettings& settingsNew, const HttpServerSettings& httpSettingsNew)
	{
		Agent::applySettings(settingsNew);
		httpSettings = std::make_shared<HttpServerSettings>(httpSettingsNew);
	}

	void HttpServer::start(std::string_view address, std::string_view port)
	{
#ifdef _WIN32
		ESLog::es_detail(ESLog::FormatStr() << "Server starting");
#else
		ESLog::es_detail(ESLog::FormatStr() << "Server starting. Executable path: " << std::filesystem::canonical("/proc/self/exe"));
#endif
		const auto listenPort = port.empty() ? ((httpMode == HttpMode::HTTPS) ? "443" : "80") : port;
		Agent::listen(listenPort, address);
	}

	void HttpServer::handleRequests()
	{
		Agent::updateConnections();
		httpFilesystem.refreshTimed(httpSettings->filesystemRefreshIntervalSec);

		for (Connection& conn : Agent::getAllConnections())
		{
			if (conn.getIncomingDataSize() >= 26)
			{
				//ESLog::es_detail(ESLog::FormatStr() << "Incoming " << conn.getIncomingDataSize() << " bytes");
				if (ES_ENABLE_HTTPSRV_THREADING)
					futures.push_back(std::async(std::launch::async, &HttpServer::handleHttpRequest, std::ref(conn), std::ref(handlers)));
				else
					HttpServer::handleHttpRequest(conn, handlers);

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
				ESLog::es_detail(ESLog::FormatStr() << "Processed request in " << result.timeTakenToCompleteMs << "ms" 
										<< "\n{ \n\t" << result.request.toShortString() << "\n }\n" << status << "\n");
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
		Timer requestCompletionTimer{};
		requestCompletionTimer.start();
		
		std::string requestString;
		RequestCompleteness completeness = RequestCompleteness::PARTIAL;
		// parse request in pieces, and impose a timeout to mitigate "low and slow" clients
		while (completeness == RequestCompleteness::PARTIAL and (requestCompletionTimer.getElapsed() < 3.f))
		{
			connection.receive(requestString);
			completeness = InputHandler::getHttpRequestCompleteness(requestString);
		}

		if (requestString.empty() or completeness == RequestCompleteness::BAD)
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST, .request = {}, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs() };

		HttpStatusCode parserStatus = HttpStatusCode::SRV_ERROR;
		HttpRequest request = InputHandler::parseHttpRequestSafe(requestString, parserStatus);

		// return early for issues found during parsing
		if (httpStatusCodeIsError(parserStatus) or request.method == HttpMethodType::UNRECOGNIZED_M)
		{
			if (parserStatus == HttpStatusCode::URI_TOO_LONG or 
				parserStatus == HttpStatusCode::PAYLOAD_TOO_LARGE or 
				parserStatus == HttpStatusCode::NO_REQUEST_LENGTH)
				return HttpTaskResult{ .statusCode = parserStatus, .request = {}, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs()};
			if (request.method == HttpMethodType::UNRECOGNIZED_M)
				return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .request = {}, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs() };
			return HttpTaskResult{ .statusCode = HttpStatusCode::BAD_REQUEST, .request = {}, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs() };
		}
		
		for (HttpHandlerBinding& handler : methodHandlers)
		{
			if (handler.method == request.method or handler.method == HttpMethodType::ANY_M)
			{
				const HttpResponse response = handler.execute(request);
				if (not response.handled)
					continue; // handler refused to process the request, try other handlers

				std::string responseString = response.finalizeToString();

				connection.send(responseString);

				return HttpTaskResult{ .statusCode = response.statusCode, .request = request, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs() };
			}
		}

		return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .request = {}, .timeTakenToCompleteMs = requestCompletionTimer.getElapsedMs() };
	}

	
	HttpResponse HttpServer::filesystemRequestHandler(const HttpRequest& request) const
	{
		if (request.method != HttpMethodType::GET_M)
			return HttpResponse::errorResponse(HttpStatusCode::METHOD_NOT_ALLLOWED);

		// in dynamic mode, requests might be to rehydrate a page, or just part of a page instead of a whole file
		ESLog::es_detail(ESLog::FormatStr() << "Getting file info for " << request.url);
		FileFormatInfo requestFileInfo = httpFilesystem.fileFormatFromPath(request.url);

		if (serverMode == ServerMode::Dynamic and 
			(requestFileInfo.extensionEnum == CommonFileExt::NONE or
			requestFileInfo.extensionEnum == CommonFileExt::HTML))
		{
			ESLog::es_detail(ESLog::FormatStr() << "Request for " << request.url << " passed to dynamic request handler");
			return dynamicRequestHandler(request);
		}

		// serve a static file, usually a full page reload clientside
		const auto fileId = httpFilesystem.findFile(request.url);
		if (not fileId)
			return HttpResponse::errorResponse(HttpStatusCode::NOT_FOUND);

		auto fileInfo = httpFilesystem.getFileInfo(fileId);
		std::string content;
		if (not httpFilesystem.getFileAsString(fileId, content))
			return HttpResponse::errorResponse(HttpStatusCode::SRV_ERROR);
		if (content.empty())
			return HttpResponse::errorResponse(HttpStatusCode::NO_CONTENT);

		return HttpResponse
		{
			.statusCode = HttpStatusCode::OK,
			.headerFields = 
				{
					httpFilesystem.makeContentTypeHeaderField(fileInfo.knownExtension)
				},
			//{ "Content-Type: text/html; charset=utf-8" }, // TODO: determine content type
			.payload = content
		};
	}

	HttpResponse HttpServer::dynamicRequestHandler(const HttpRequest& request) const
	{
		if (request.getHeaderFieldValue("X-Requested-With") != "SPA")
		{
			// serve a blank bootstrapping page
			auto page = makeDynamicBootstrapPage(request.url);
			return HttpResponse
			{
				.statusCode = HttpStatusCode::OK,
				.headerFields = { httpFilesystem.makeContentTypeHeaderField("html") },
				.payload = page
			};
		}

		std::string url = request.url;
		if (url[0] == '/')
			url = url.substr(1);
		const auto fileId = httpFilesystem.findFile(request.url);
		if (not fileId)
			return HttpResponse::errorResponse(HttpStatusCode::NOT_FOUND);

		auto fileInfo = httpFilesystem.getFileInfo(fileId);
		std::string content;
		if (not httpFilesystem.getFileAsString(fileId, content))
			return HttpResponse::errorResponse(HttpStatusCode::SRV_ERROR);
		if (content.empty())
			return HttpResponse::errorResponse(HttpStatusCode::NO_CONTENT);

		makeHtmlDynamicPage(content, request.url);

		return HttpResponse
		{
			.statusCode = HttpStatusCode::OK,
			.headerFields =
				{
					httpFilesystem.makeContentTypeHeaderField(fileInfo.knownExtension)
				},
			.payload = content
		};
	}

}
