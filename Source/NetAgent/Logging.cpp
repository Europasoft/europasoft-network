// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "NetAgent/Logging.h"
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

	std::string getLogLevelString(Lvl lvl)
	{
		if (lvl == Lvl::ES_TRAFFIC)
			return "TRAFFIC";
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
		std::time_t tnow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char tbuf[30];
		ctime_s(tbuf, sizeof(tbuf), &tnow);
		return "\n" + getLogLevelString(logLevel) + " [" + tbuf + "] \t" + message;
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

	void logToOutput(std::string message)
	{
		if (getGlobalLogSettings().enableLogToOutput and not getGlobalLogSettings().disableAllLogging)
			std::cout << message;
	}

	void logToFileAndOutputAsync(std::string message)
	{
		std::thread([message]()
			{
				WIN_SET_THREAD_NAME(L"Log async");
				getLogMutex().lock();
				logToFile(message);
				logToOutput(message);
				getLogMutex().unlock();
			}).detach();
	}

	void es_traffic(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_TRAFFIC, message);
	}

	void es_info(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_INFO, message);
	}

	void es_warning(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_WARNING, message);
	}

	void es_error(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_ERROR, message);
		assert(0 && message.c_str());
	}

	void es_fatal(std::string message)
	{
		ESLog_LOG_MSG(Lvl::ES_FATAL, message);
		assert(0 && message.c_str());
		throw std::runtime_error(message);
	}

	void es_assertRuntime(bool condition, std::string message)
	{
		if (not condition)
			es_fatal(message);
	}


}


