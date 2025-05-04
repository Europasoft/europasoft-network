// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iomanip>

namespace ESLog
{
	enum class Lvl : uint32_t
	{
		ES_DETAIL = 0,
		ES_INFO = 1,
		ES_WARNING = 2,
		ES_ERROR = 3,
		ES_FATAL = 4
	};

	struct GlobalLogSettings
	{
		Lvl logLevel = Lvl::ES_INFO;
		bool enableLogToFile = true;
		bool enableLogToOutput = true;
		bool disableAllLogging = false;
	};

	GlobalLogSettings& getGlobalLogSettings();

	void setGlobalLogSettings(GlobalLogSettings settings);

	std::mutex& getLogMutex();

	#define ESLog_LOG_MSG(level, message) \
	if (static_cast<uint32_t>(level) >= static_cast<uint32_t>(getGlobalLogSettings().logLevel))\
	{\
		message = formatLogMessage(level, message);\
		logToFileAndOutputAsync(message);\
	}

	std::string getLogLevelString(Lvl lvl);

	std::string formatLogMessage(Lvl logLevel, std::string message);

	void logToFile(std::string message);

	void logToOutput(std::string message);

	void es_detail(std::string message);

	void es_info(std::string message);

	void es_warning(std::string message);

	void es_error(std::string message);

	void es_fatal(std::string message);

	void es_assertRuntime(bool condition, std::string message);

	// this makes string formatting syntax a bit cleaner, in absence of std::format
	struct FormatStr
	{
		std::ostringstream strs{};

		template <typename T>
		FormatStr& operator<<(T v)
		{
			strs << v;
			return *this;
		}
		FormatStr& operator<<(double v)
		{
			strs << std::setprecision(17) << v;
			return *this;
		}

		operator std::string()
		{
			return strs.str();
		}
	};

}


