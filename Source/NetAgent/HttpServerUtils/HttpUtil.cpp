// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "NetAgent/HttpServer.h"
#include "NetAgent/HttpServerUtils/HttpUtil.h"
#include "NetAgent/HttpServerUtils/Logging.h"

#include <stdint.h>
#include <fstream>
#include <algorithm>


namespace HTTP
{
	constexpr size_t ES_URI_LIMIT = 9000;

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
		headerFields.push_back(ESLog::FormatStr() << name << ((name.ends_with(':')) ? "" : ":") << value);
	}

	std::string HttpResponse::finalizeToString() const
	{
		HttpResponse res = *this;
		res.headerFields.push_back(ESLog::FormatStr() << "Content-Length: " << res.payload.size());

		std::string header{};
		for (auto& field : res.headerFields)
			header.append(ESLog::FormatStr() << field << "\r\n");

		return ESLog::FormatStr() << makeResponseVersionString() << " " << makeResponseStatusCodeString(res.statusCode)
					<< "\r\n" << header << "\r\n" << res.payload;
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

	HttpResponse HttpResponse::unhandledResponse()
	{
		HttpResponse response{};
		response.handled = false;
		return response;
	}

	std::string makeResponseVersionString()
	{
		return "HTTP/1.1";
	}

	std::string makeResponseStatusCodeString(HttpStatusCode code)
	{
		return ESLog::FormatStr() << static_cast<uint32_t>(code) << " " << httpStatusCodeToString(code);
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
		if (not webRootPath.empty())
			webroot = webRootPath;
		if (webroot.empty())
			return;
		
		updateContentTypeMappings();
		for (const auto& p : std::filesystem::recursive_directory_iterator(webroot))
		{
			if (not std::filesystem::is_directory(p))
			{
				const auto full = std::filesystem::weakly_canonical(webroot / p.path());
				const auto rel = std::filesystem::relative(full, webroot);
				if (full.is_relative() or (not full.is_absolute()) or rel.is_absolute() or (not rel.is_relative()))
					continue;
				allowedFilepaths.push_back(PathInfo
					{
						.relative = std::filesystem::relative(p.path(), webroot),
						.full = std::filesystem::weakly_canonical(webroot / p.path()),
						.knownExtension = p.path().extension().string()
					});
				if (not filesystemRefreshTimer)
					ESLog::es_detail(ESLog::FormatStr() << allowedFilepaths.back().relative << " (" << makeContentTypeHeaderField(allowedFilepaths.back().knownExtension) << ")");
			}
		}
		ESLog::es_info("Refreshed filesystem paths under WebRoot");

		if (filesystemRefreshTimer)
			filesystemRefreshTimer->start();
		else
			filesystemRefreshTimer = std::make_unique<Timer>();
	}

	void HttpFilesystem::refreshTimed(double intervalSeconds)
	{
		if (webroot.empty())
			return;
		if (filesystemRefreshTimer->getElapsed() >= intervalSeconds)
			updateFullRefresh("");
	}

	void HttpFilesystem::updateContentTypeMappings()
	{
		HttpFilesystem::fileExtensionContentTypeMappings =
		{
			// only includes the most common file types
			FileFormatInfo{ CommonFileExt::HTML,	"html",		"text/html",		ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::CSS,		"css",		"text/css",			ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::JS,		"js",		"text/javascript",	ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::JSON,	"json",		"application/json",	ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::CSV,		"csv",		"text/csv",			ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::TXT,		"txt",		"text/plain",		ContentTypeCategory::U8TEXT},
			FileFormatInfo{ CommonFileExt::PNG,		"png",		"image/png",		ContentTypeCategory::BINARY},
			FileFormatInfo{ CommonFileExt::SVG,		"svg",		"image/svg+xml",	ContentTypeCategory::BINARY},
			FileFormatInfo{ CommonFileExt::WEBP,	"webp",		"image/webp",		ContentTypeCategory::BINARY}
		};
	}

	std::string HttpFilesystem::normalizePath(const std::filesystem::path& original) const
	{
		std::string normalized = original.string();
		std::replace(normalized.begin(), normalized.end(), '\\', '/');
		if (normalized.length() > 0 and normalized.at(0) != '/')
			normalized = "/" + normalized;
		return normalized;
	}

	size_t HttpFilesystem::findFile(const std::filesystem::path& path) const
	{
		for (size_t i = 0; i < allowedFilepaths.size(); i++)
		{
			const auto& relative = allowedFilepaths[i].relative;
			const std::string relativeNormalized = normalizePath(relative);
			if (relative == path or relativeNormalized == path.string())
				return i + 1;
			// if the url is a directory "x" which contains an "index.html" or "x.html", return that file
			const std::string lastDirName = relative.parent_path().filename().string();
			const std::string speculativeIndexPath = normalizePath(path / "index.html");
			const std::string speculativeDirNamePath = normalizePath(path / (lastDirName + ".html"));
 			if (relativeNormalized == speculativeIndexPath or relativeNormalized == speculativeDirNamePath)
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
		const auto& fullPath = getFileInfo(id).full;

		if (not (fullPath.is_absolute() and std::filesystem::is_regular_file(fullPath)))
			return false;

		contentOut = fileToString(fullPath);
		return true;
	}

	HttpFilesystem::PathInfo HttpFilesystem::getFileInfo(size_t id) const
	{
		return allowedFilepaths[id - 1];
	}

	std::string HttpRequest::toShortString() const
	{
		return ESLog::FormatStr() << httpMethodToString(method) << " " << url.substr(0, 300) << (url.length() > 60 ? "..." : "");
	}

	std::string HttpRequest::getHeaderFieldValue(const std::string& name) const
	{
		for (const std::string& headerField : headerFields)
		{
			if (headerField.length() < name.length() + 2 or headerField.substr(0, name.length()) != name)
				continue;

			std::string field = headerField.substr(name.length());
			auto valueStart = field.find(":");
			if (valueStart == std::string::npos or valueStart + 1 >= field.length())
				return std::string();
			field = field.substr(valueStart + 1);
			valueStart = field.find_first_not_of(" ");
			if (valueStart == std::string::npos)
				return std::string();

			return field.substr(valueStart);
		}
		return std::string();
	}

	FileFormatInfo HttpFilesystem::fileFormatFromExtension(std::string fileExtension) const
	{
		if (fileExtension[0] != '.')
			fileExtension = ESLog::FormatStr() << "." << fileExtension;
		auto& mappings = HttpFilesystem::fileExtensionContentTypeMappings;
		auto iterator = std::find_if(mappings.begin(), mappings.end(),
								[&](const auto& b) { return (fileExtension == ("." + b.extensionString)); });

		if (iterator != mappings.end())
			return *iterator;
		else
			return FileFormatInfo{ CommonFileExt::NONE };
	}

	FileFormatInfo HttpFilesystem::fileFormatFromPath(std::string path) const
	{
		return fileFormatFromExtension(getFileExtension(path));
	}

	std::string HttpFilesystem::getFileExtension(std::string path) const
	{
		size_t dotPos = path.find_last_of('.');
		if (dotPos != std::string::npos)
			return path.substr(dotPos + 1);
		else
			return std::string();
	}

	std::string HttpFilesystem::makeContentTypeHeaderField(std::string fileExtension) const
	{
		FileFormatInfo info = fileFormatFromExtension(fileExtension);
		if (info.extensionEnum == CommonFileExt::NONE)
			info = { CommonFileExt::TXT, "txt", "text/plain", ContentTypeCategory::U8TEXT };

		return ESLog::FormatStr() << "Content-Type: " << info.mediaTypeString 
					<< (info.contentTypeCategory == ContentTypeCategory::U8TEXT ? "; charset=utf-8" : "");
	}

	void replaceSubstring(std::string& string, const std::string& from, const std::string& to)
	{
		auto index = string.find(from);
		while (index != std::string::npos)
		{
			string.replace(index, from.length(), to);
			index = string.find(from, index + to.length());
		}
	}

	HttpRequest InputHandler::parseHttpRequestSafe(const std::string& request, HttpStatusCode& parserStatusOut)
	{
		try
		{
			return InputHandler::parseHttpRequest(request, parserStatusOut);
		}
		catch (...)
		{
			parserStatusOut = HttpStatusCode::BAD_REQUEST;
			return HttpRequest{};
		}
	}

	RequestCompleteness InputHandler::getHttpRequestCompleteness(const std::string& request)
	{
		auto methodEnd = request.find(" ");
		if (methodEnd == std::string::npos)
		{
			// end of method not found
			if (request.length() >= 8)
				return RequestCompleteness::BAD; // request not valid, must have whitespace within the first 8 bytes
			else
				return RequestCompleteness::PARTIAL;
		}
		const auto method = httpMethodFromString(request.substr(0, methodEnd));
		if (method == HttpMethodType::UNRECOGNIZED_M)
			return RequestCompleteness::BAD; // request not valid, unrecognized method

		// method is complete

		auto urlEnd = request.find(" ", methodEnd + 1);
		if (urlEnd == std::string::npos)
		{
			// end of url not found
			if ((request.length() - methodEnd) > (ES_URI_LIMIT + 9))
				return RequestCompleteness::BAD; // request not valid, url is too long
			else
				return RequestCompleteness::PARTIAL;
		}

		// url is complete

		const bool completeHeader = (request.find("\r\n") != std::string::npos);
		if (not completeHeader)
		{
			// end of header not found
			if ((request.length() - urlEnd) >= 14)
				return RequestCompleteness::BAD; // request not valid, no end of header
			else
				return RequestCompleteness::PARTIAL;
		}

		return RequestCompleteness::FULL;
	}

	HttpRequest InputHandler::parseHttpRequest(const std::string& request, HttpStatusCode& parserStatusOut)
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

		HttpRequest req;
		req.method = httpMethodFromString(request.substr(0, methodEnd));
		req.url = request.substr(methodEnd, urlEnd - methodEnd);
		std::erase(req.url, ' ');

		// TODO: check payload length and request length value in request
		auto lastFieldEnd = urlEnd;
		for (uint32_t i = 0; i < 200; i++)
		{
			auto fieldStart = request.find("\r\n", lastFieldEnd);
			auto fieldEnd = request.find("\r\n", fieldStart + 1);
			auto field = request.substr(fieldStart, fieldEnd - fieldStart);
			if (not (field == "\r\n" or field.empty()))
			{
				replaceSubstring(field, "\r", "");
				replaceSubstring(field, "\n", "");
				req.headerFields.push_back(field);
				lastFieldEnd = fieldEnd;
			}
			else
			{
				break;
			}
		}

		parserStatusOut = HttpStatusCode::OK;
		return req;

	}


}
