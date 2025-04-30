// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpUtil.h"
#include "NetAgent/Logging.h"

#include <stdint.h>
#include <fstream>
#include <algorithm>

namespace HTTP
{

	//const char* es_stringFormatArg(const std::string& s, std::vector<std::string*>& allocs)
	//{
	//	return s.c_str();
	//}

	//const char* es_stringFormatArg(int64_t i, std::vector<std::string*>& allocs)
	//{
	//	allocs.push_back(new std::string(std::to_string(i)));
	//	return allocs.back()->c_str();
	//}

	//const char* es_stringFormatArg(double d, std::vector<std::string*>& allocs)
	//{
	//	allocs.push_back(new std::string(std::to_string(d)));
	//	return allocs.back()->c_str();
	//}


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

	std::string httpStatusCodeToString(HttpStatusCode code)
	{
		auto& mappings = StringEnumHelpers::httpStatusCodeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.first == code); });
		return (iterator != mappings.end()) ? (*iterator).second : "UNRECOGNIZED CODE";
	}

	bool httpStatusCodeIsError(HttpStatusCode code)
	{
		return static_cast<uint32_t>(code) >= 400;
	}

	HttpMethodType httpMethodFromString(std::string str)
	{
		auto& mappings = StringEnumHelpers::httpMethodTypeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.second[0] == str[0] and p.second[1] == str[1]); });
		return (iterator != mappings.end()) ? (*iterator).first : HttpMethodType::UNRECOGNIZED_M;
	}

	std::string httpMethodToString(HttpMethodType method)
	{
		auto& mappings = StringEnumHelpers::httpMethodTypeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
			[&](const auto& p) { return (p.first == method); });
		return (iterator != mappings.end()) ? (*iterator).second : "UNRECOGNIZED METHOD";
	}


	void HttpResponse::addHeaderField(std::string_view name, std::string_view value)
	{
		if (name.substr(0, 15) == "Content-Length")
		{
			ESLog::es_warning("Manually adding Content-Length header field to HTTP responses is not necessary");
			return;
		}
		//headerFields.push_back(formatStr("%s%s%s", name, (name.ends_with(':')) ? "" : ":", value));
		headerFields.push_back((std::ostringstream() << name << ((name.ends_with(':')) ? "" : ":") << value).str());
	}

	std::string HttpResponse::finalizeToString() const
	{
		HttpResponse res = *this;
		//res.headerFields.push_back(formatStr("Content-Length: %s", res.payload.size()));
		res.headerFields.push_back((std::ostringstream() << "Content-Length: " << res.payload.size()).str());
		std::string header{};
		for (size_t i = 1; i < res.headerFields.size(); i++)
			//header.append(formatStr("{}\r\n", res.headerFields[i]));
			header.append((std::ostringstream() << res.headerFields[i] << "\r\n").str());

		//return formatStr("%s %s\r\n%s\r\n%s", makeResponseVersionString(), makeResponseStatusCodeString(res.statusCode), header, res.payload);
		return (std::ostringstream() << makeResponseVersionString() << " " << makeResponseStatusCodeString(res.statusCode) 
					<< "\r\n" << header << "\r\n" << res.payload).str();
	}

	HttpResponse HttpResponse::errorResponse(HttpStatusCode code)
	{
		return HttpResponse
		{
			.statusCode = code,
			.headerFields = { "Content-Type: text/html; charset=utf-8" },
			.payload = makeResponseStatusCodeString(code)
		};
	}

	std::string makeResponseVersionString()
	{
		return "HTTP/1.1";
	}

	std::string makeResponseStatusCodeString(HttpStatusCode code)
	{
		//return formatStr("%s %s", static_cast<uint32_t>(code), httpStatusCodeToString(code));
		return (std::ostringstream() << static_cast<uint32_t>(code) << " " << httpStatusCodeToString(code)).str();
	}

	std::string fileToString(const std::filesystem::path& filepath)
	{
		std::ifstream t(filepath);
		if (not t.good())
			return "";
		t.seekg(0, std::ios::end);
		size_t size = t.tellg();
		std::string buffer(size, ' ');
		t.seekg(0);
		t.read(&buffer[0], size);
		return buffer;
	}

	void HttpFilesystem::updateFullRefresh(std::string_view webRootPath)
	{
		ESLog::es_info("Refreshing filesystem paths under WebRoot");
		webroot = webRootPath;
		for (const auto& p : std::filesystem::recursive_directory_iterator(webRootPath))
		{
			if (not std::filesystem::is_directory(p))
			{
				const auto full = std::filesystem::weakly_canonical(webroot / p.path());
				const auto rel = std::filesystem::relative(full, webRootPath);
				if (full.is_relative() or (not full.is_absolute()) or rel.is_absolute() or (not rel.is_relative()))
					continue;
				allowedFilepaths.push_back(PathInfo
					{
						.relative = std::filesystem::relative(p.path(), webRootPath),
						.full = std::filesystem::weakly_canonical(webroot / p.path())
					});
			}
		}
		ESLog::es_info("Completed gathering filesystem paths");
	}

	size_t HttpFilesystem::findFile(const std::filesystem::path& path) const
	{
		for (size_t i = 0; i < allowedFilepaths.size(); i++)
		{
			auto relativeNormalized = allowedFilepaths[i].relative.string();
			std::replace(relativeNormalized.begin(), relativeNormalized.end(), '\\', '/');
			relativeNormalized = "/" + relativeNormalized;
			// TODO: this part doesn't work although paths seem to match - why?
			if (allowedFilepaths[i].relative == path or relativeNormalized == path.string())
				return i + 1;
		}
		return 0;
	}

	bool HttpFilesystem::getFileAsString(size_t id, std::string& contentOut) const
	{
		if (id < 1 or id > allowedFilepaths.size())
		{
			ESLog::es_error("Attempted to read file with bad id");
			return false;
		}
		const auto& fullPath = allowedFilepaths[id - 1].full;

		if (not (fullPath.is_absolute() and std::filesystem::is_regular_file(fullPath)))
			return false;

		contentOut = fileToString(fullPath);
		return true;
	}

}
