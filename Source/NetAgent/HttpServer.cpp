// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include <iostream>

namespace HTTP
{
	std::array<std::pair<HttpStatusCode, std::string>, 24> StringEnumHelpers::httpStatusCodeMappings =
		{
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::OK,							"OK" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::CREATED,					"Created" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::ACCEPTED,					"Accepted" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::NO_CONTENT,					"No Content" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::REDIRECT,					"Temporary Redirect" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::REDIRECT_PERMANENT,			"Permanent Redirect" },

			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::BAD_REQUEST,				"Bad Request" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::UNAUTHORIZED,				"Unauthorized" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::FORBIDDEN,					"Forbidden" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::NOT_FOUND,					"Not Found" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::METHOD_NOT_ALLLOWED,		"Method Not Allowed" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::NO_ACCEPTABLE_CONTENT,		"Not Acceptable" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::TIMEOUT,					"Request Timeout" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::NO_REQUEST_LENGTH,			"Length Required" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::PAYLOAD_TOO_LARGE,			"Payload Too Large" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::URI_TOO_LONG,				"URI Too Long" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::UNSUPPORTED_MEDIA_TYPE,		"Unsupported Media Type" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::IM_A_TEAPOT,				"I'm a teapot" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::TOO_MANY_REQUESTS,			"Too Many Requests" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::UPGRADE_REQUIRED,			"Upgrade Required" },

			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::SRV_ERROR,					"Internal Server Error" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::SRV_NOT_IMPLEMENTED,		"Not Implemented" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::SRV_TEMPORARILY_UNAVAILABLE,"Service Unavailable" },
			std::pair<HttpStatusCode, std::string>{ HttpStatusCode::HTTP_VERSION_UNSUPPORTED,	"HTTP Version Not Supported" },
		};

	std::array<std::pair<HttpMethodType, std::string>, 9> StringEnumHelpers::httpMethodTypeMappings =
		{
			std::pair<HttpMethodType, std::string>{ HttpMethodType::GET_M,		"GET" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::HEAD_M,		"HEAD" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::POST_M,		"POST" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::PUT_M,		"PUT" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::DELETE_M,	"DELETE" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::CONNECT_M,	"CONNECT" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::OPTIONS_M,	"OPTIONS" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::TRACE_M,	"TRACE" },
			std::pair<HttpMethodType, std::string>{ HttpMethodType::PATCH_M,	"PATCH" }
		};


	HttpServer::HttpServer(HttpServer::HttpMode httpMode)
		: Agent{ Agent::Mode::Server }, httpMode{ httpMode }
	{
		// TODO: HTTPS (TLS) support
		assert(httpMode != HttpMode::HTTPS);
	}

	void HttpServer::bindRequestHandler(HttpMethodType httpMethod, std::function<HttpResponse(const HttpRequest&)> handlerFunction)
	{
		handlers.push_back(HttpHandlerBinding{ .method = httpMethod, .execute = handlerFunction });
	}

	void HttpServer::start(std::string_view address)
	{
		const auto port = httpMode == HttpMode::HTTPS ? "443" : "80";
		Agent::listen(port, address);
	}

	void HttpServer::handleRequests()
	{
		Agent::updateConnections();

		for (Connection& conn : Agent::getAllConnections())
		{
			if (conn.getIncomingDataSize() > 0)
			{
				std::cout << "\nIncoming " << conn.getIncomingDataSize() << " bytes";
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
				std::cout << "\nRequest\n'" << result.originalRequest << "'\nProcessed: '" << ((uint32_t)result.statusCode) << httpStatusCodeToString(result.statusCode) << "'";
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
			if (handler.method == request.method)
			{
				const HttpResponse response = handler.execute(request);
				// temporary
				std::string responseString = response.payload;
				connection.send(responseString);
				return HttpTaskResult{ .statusCode = response.statusCode, .originalRequest = requestString, .logInfo = "handled" };
			}
		}

		return HttpTaskResult{ .statusCode = HttpStatusCode::METHOD_NOT_ALLLOWED, .originalRequest = requestString };
	}

	HttpRequest HttpServer::parseHttpRequest(const std::string& request)
	{
		auto methodEnd = request.find(" ");
		auto urlEnd = request.find(" ", methodEnd);
		HttpRequest req;
		req.method = httpMethodFromString(request.substr(0, methodEnd));
		req.url = request.substr(methodEnd, urlEnd);
		return req;
	}


	const std::string& httpStatusCodeToString(HttpStatusCode code)
	{
		auto& mappings = StringEnumHelpers::httpStatusCodeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.first == code); });
		return (iterator != mappings.end()) ? (*iterator).second : "UNRECOGNIZED CODE";
	}

	HttpMethodType httpMethodFromString(const std::string& str)
	{
		auto& mappings = StringEnumHelpers::httpMethodTypeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.second[0] == str[0] and p.second[1] == str[1]); });
		return (iterator != mappings.end()) ? (*iterator).first : HttpMethodType::UNRECOGNIZED_M;
	}

	const std::string& httpMethodToString(HttpMethodType method)
	{
		auto& mappings = StringEnumHelpers::httpMethodTypeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.first == method); });
		return (iterator != mappings.end()) ? (*iterator).second : "UNRECOGNIZED METHOD";
	}


}
