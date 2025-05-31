// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "NetAgent/HttpServerUtils/Logging.h"
#include "Sockets/PlatformMacros.h"
#include <thread>

namespace ESLog
{

	GlobalLogSettings& getGlobalLogSettings()
	{
		static GlobalLogSettings gls{};
		return gls;
	}

	void setGlobalLogSettings(GlobalLogSettings settings)
	{
		getGlobalLogSettings() = settings;
	}

	std::mutex& getLogMutex()
	{
		static std::mutex m;
		return m;
	}

	std::string colorLogMesage(const char* color, std::string msg)
	{
		return (color + msg + ES_LOG_RESET);
	}

	std::string getLogLevelString(Lvl lvl)
	{
		if (lvl == Lvl::ES_DETAIL)
			return "DETAIL";
		else if (lvl == Lvl::ES_INFO)
			return "INFO";
		else if (lvl == Lvl::ES_WARNING)
			return "WARNING";
		else if (lvl == Lvl::ES_ERROR)
			return "ERROR";
		else
			return "FATAL";
	}

	std::string formatLogMessage(Lvl logLevel, std::string message)
	{
		return FormatStr() << "\n[" << getLogLevelString(logLevel) << "] " << message;
	}

	void logToFile(std::string message)
	{
		if (getGlobalLogSettings().enableLogToFile and not getGlobalLogSettings().disableAllLogging)
		{
			std::ofstream lf;
			lf.open("log.txt");
			if (lf.is_open() and lf.good())
			{
				lf << "\n" << message;
				lf.close();
			}
		}
	}

	void logToOutput(std::string message, const char* color)
	{
		if (getGlobalLogSettings().enableLogToOutput and not getGlobalLogSettings().disableAllLogging)
			std::cout << colorLogMesage(color, message);
	}

	void logToFileAndOutputAsync(std::string message, const char* color)
	{
		std::thread([message, color]()
			{
				WIN_SET_THREAD_NAME(L"Log async");
				if (getLogMutex().try_lock())
				{
					logToFile(message);
					logToOutput(message, color);
					getLogMutex().unlock();
				}
			}).detach();
	}

	void es_detail(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_DETAIL, message, ES_LOG_WHITE);
	}

	void es_info(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_INFO, message, ES_LOG_CYAN);
	}

	void es_warning(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_WARNING, message, ES_LOG_YELLOW);
	}

	void es_error(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_ERROR, message, ES_LOG_RED);
	}

	void es_fatal(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_FATAL, message, ES_LOG_BRIGHT_RED);
		assert(0 && message.c_str());
		throw std::runtime_error(message);
	}

	void es_assertRuntime(bool condition, std::string message)
	{
		if (not condition)
			es_fatal(message);
	}
}


