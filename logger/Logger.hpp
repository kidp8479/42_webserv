#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "../utils/colors.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

class LogLine;

/**
 * @class Logger
 * @brief Singleton logger supporting multiple log levels and RAII.
 * (Resource Acquisition Is Initialization.)
 *
 * Logger manages a single logging instance, allowing messages to be
 * streamed via LogLine objects. It supports thresholds to filter
 * messages, converting string levels to enum, and flushes messages
 * to stdout with color-coded prefixes.
 */
class Logger
{
public:
	/* Supported log levels */
	enum logLevel
	{
		INVALID = -1,
		NONE = 0,
		DEBUG = 1,
		INFO = 2,
		WARNING = 3,
		ERROR = 4,
	};

	/* Singleton instance */
	static		Logger& get();
	
	/* Flush a completed message */
	void		flush(const std::string& prefix,
					  const std::string& message,
					  logLevel level);

	/* Set the current log threshold */
	void		setThreshold(logLevel level);
	logLevel	threshold() const;

	/* RAII factory for LogLine */
	LogLine		makeLine(const std::string& prefix, logLevel level);

	/* Convert string to logLevel enum */
	static		logLevel stringToLevel(const std::string& levelStr);

	void		setLogFile(const std::string& filename);

private:
	// prevents anyone from instantianting a logger object
	// only the logger class itself is allowed to do that
	// through Logger& get()
	Logger() : _threshold(NONE), _useFile(false) {}
	~Logger() {}
	Logger(const Logger&);
	Logger& operator=(const Logger&);

	static std::string	getTimeStamp();

	logLevel		_threshold;
	std::ofstream	_file;
	bool			_useFile;
};

/**
 * @class LogLine
 * @brief RAII helper to build and flush a log message.
 *
 * LogLine accumulates streamed values into an internal buffer. When
 * destroyed, it automatically flushes the message through the
 * associated Logger, respecting the log level and threshold.
 */
class LogLine
{
public:
	LogLine(Logger& logger, const std::string& prefix, Logger::logLevel level);
	LogLine(const LogLine& other);
	~LogLine();

	template<typename T>
	LogLine& operator<<(const T& value)
	{
		_stream << value;
		return (*this);
	}

private:
	LogLine& operator=(const LogLine&); // prevent assignment

	Logger&				_logger;
	std::ostringstream	_stream;
	std::string			_prefix;
	Logger::logLevel			_level;
};

/* logging macros */
#define LOG_DEBUG()		Logger::get().makeLine(yel("[DEBUG] "), Logger::DEBUG)
#define LOG_INFO()		Logger::get().makeLine(blu("[INFO] "), Logger::INFO)
#define LOG_WARNING()	Logger::get().makeLine(mag("[WARNING] "), Logger::WARNING)
#define LOG_ERROR()		Logger::get().makeLine(red("[ERROR] "), Logger::ERROR)

bool initLogger(const std::string& levelStr);

#endif
