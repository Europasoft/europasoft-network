// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "NetAgent/Agent.h"
#include <stdint.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <array>
#include <list>
#include <cassert>
#include <functional>
#include <future>

namespace HTTP
{
	enum class HttpStatusCode : uint32_t
	{
		UNRECOGNIZED = 99,
		OK = 200,
		CREATED = 201,
		ACCEPTED = 202,
		NO_CONTENT = 204,
		REDIRECT = 307,
		REDIRECT_PERMANENT = 308,

		BAD_REQUEST = 400,
		UNAUTHORIZED = 401,
		FORBIDDEN = 403,
		NOT_FOUND = 404,
		METHOD_NOT_ALLLOWED = 405,
		NO_ACCEPTABLE_CONTENT = 406,
		TIMEOUT = 408,
		NO_REQUEST_LENGTH = 411,
		PAYLOAD_TOO_LARGE = 413,
		URI_TOO_LONG = 414,
		UNSUPPORTED_MEDIA_TYPE = 415,
		IM_A_TEAPOT = 418,
		TOO_MANY_REQUESTS = 429,
		UPGRADE_REQUIRED = 426,

		SRV_ERROR = 500,
		SRV_NOT_IMPLEMENTED = 501,
		SRV_TEMPORARILY_UNAVAILABLE = 503,
		HTTP_VERSION_UNSUPPORTED = 505
	};

	const std::string& httpStatusCodeToString(HttpStatusCode code);

	enum class HttpMethodType : uint32_t
	{
		GET_M = 0,
		HEAD_M = 1,
		POST_M = 2,
		PUT_M = 3,
		DELETE_M = 4,
		CONNECT_M = 5,
		OPTIONS_M = 6,
		TRACE_M = 7,
		PATCH_M = 8,
		UNRECOGNIZED_M = 99,
	};

	HttpMethodType httpMethodFromString(const std::string& str);
	const std::string& httpMethodToString(HttpMethodType method);

	struct StringEnumHelpers
	{
		static std::array<std::pair<HttpStatusCode, std::string>, 24> httpStatusCodeMappings;
		static std::array<std::pair<HttpMethodType, std::string>, 9> httpMethodTypeMappings;
	};

	struct HttpRequest
	{
		HttpMethodType method = HttpMethodType::UNRECOGNIZED_M;
		std::string url{};
		std::string headerFields{};
		std::string payload{};
	};

	struct HttpResponse
	{
		HttpStatusCode statusCode = HttpStatusCode::OK;
		std::string payload{};
	};

	struct HttpTaskResult
	{
		HttpStatusCode statusCode = HttpStatusCode::BAD_REQUEST;
		std::string originalRequest{};
		std::string logInfo{};
	};

	struct HttpHandlerBinding
	{
		HttpMethodType method = HttpMethodType::UNRECOGNIZED_M;
		std::function<HttpResponse(const HttpRequest&)> execute{};
	};


	// HTTP server agent
	class HttpServer : public Agent
	{
	public:
		enum class HttpMode { HTTP, HTTPS };
		HttpServer(HttpMode httpMode);

		void bindRequestHandler(HttpMethodType httpMethod, std::function<HttpResponse(const HttpRequest&)> handlerFunction);
		void start(std::string_view address);
		void handleRequests();
		

	protected:
		HttpMode httpMode;
		std::vector<HttpHandlerBinding> handlers;
		std::list<std::future<HttpTaskResult>> futures;
		static HttpTaskResult handleHttpRequest(Connection& connection, std::vector<HttpHandlerBinding>& methodHandlers);
		static HttpRequest parseHttpRequest(const std::string& request);

	};
}
