// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <cstdio>
#include <vector>
#include <array>
#include <utility>
#include <functional>
#include <filesystem>

#include <sstream>

namespace HTTP
{
	//struct stringFormatter
	//{
	//	std::ostringstream stream;
	//
	//	template<typename Arg_T>
	//	stringFormatter& operator<<(Arg_T arg)
	//	{
	//		stream << arg;
	//		return *this;
	//	}
	//
	//	std::string str() const
	//	{
	//		return stream.str();
	//	}
	//};

	

	//const char* es_stringFormatArg(const std::string& s, std::vector<std::string*>& allocs);

	//const char* es_stringFormatArg(int64_t i, std::vector<std::string*>& allocs);

	//const char* es_stringFormatArg(double d, std::vector<std::string*>& allocs);

	// std::format would be nicer, but not viable due to limited support
	//template<class ... Args>
	//std::string formatStr(char const* form, Args&& ...args)
	//template <class ... Ts>
	//void formatStr(char const* form, Ts && ... inputs)
	//{
	//	//std::vector<std::string> strings;
	//	//stringAllocs.reserve(30);
	//	char buf[200];
	//	std::snprintf(buf, 200, form, std::to_string(inputs) ...);
	//	//for (std::string* alloc : stringAllocs)
	//	//	delete alloc;
	//	return std::string(buf);
	//
	//	//std::string s;
	//	//([&s]
	//	//	{
	//	//		s += std::to_string(inputs);
	//	//	} (), ...);
	//	//return s;
	//}

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
		ANY_M = 101
	};
	
	struct StringEnumHelpers
	{
		static std::array<std::pair<HttpStatusCode, std::string>, 24> httpStatusCodeMappings;
		static std::array<std::pair<HttpMethodType, std::string>, 9> httpMethodTypeMappings;
	};

	std::string httpStatusCodeToString(HttpStatusCode code);
	bool httpStatusCodeIsError(HttpStatusCode code);
	
	HttpMethodType httpMethodFromString(std::string str);
	std::string httpMethodToString(HttpMethodType method);


	std::string makeResponseVersionString();
	std::string makeResponseStatusCodeString(HttpStatusCode code);

	std::string fileToString(const std::filesystem::path& filepath);

	// NOTE: this could be accelerated with a tree structure
	class HttpFilesystem
	{
		struct PathInfo { std::filesystem::path relative, full; };
		std::filesystem::path webroot{};
		std::vector<PathInfo> allowedFilepaths{};
	public:
		void updateFullRefresh(std::string_view webRootPath);

		size_t findFile(const std::filesystem::path& path) const; // paths may be matched without file extension
		bool getFileAsString(size_t id, std::string& contentOut) const;
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
		std::vector<std::string> headerFields{};
		std::string payload{};
		void addHeaderField(std::string_view name, std::string_view value);
		std::string finalizeToString() const;
		static HttpResponse errorResponse(HttpStatusCode code);
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

}
