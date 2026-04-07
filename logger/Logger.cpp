#include "Logger.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <ctime>
#include <fstream>

/*****************************************************************************
 *                                  LOGGER                                   *
 *****************************************************************************/

/**
 * @brief Get the singleton instance of Logger.
 * @return Reference to the single Logger instance.
 */
Logger& Logger::get() {
    static Logger instance;
    return (instance);
}

/**
 * @brief Get current date and time as string.
 * @return Formatted timestamp "YYYY-MM-DD HH:MM:SS".
 */
std::string Logger::getTimeStamp() {
    std::time_t now = std::time(NULL);
    std::tm* t = std::localtime(&now);

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);

    return (std::string(buffer));
}

/**
 * @brief Output a log message to console and optionally to a file.
 * @param prefix Prefix to add before the message (e.g., "[INFO] ").
 * @param message The message content to log.
 * @param level Log level of this message.
 *
 * Only messages with level >= threshold_ are printed. If a log file
 * is set, the message is also written to the file.
 */
void Logger::flush(const std::string& prefix, const std::string& message,
                   LogLevel level) {
    if (threshold_ == NONE || level < threshold_)
        return;

    std::string log_line =
        "[" + Logger::getTimeStamp() + "] " + prefix + message;

    // print to console
	if (level == ERROR)
		std::cerr << log_line << std::endl;
	else
	    std::cout << log_line << std::endl;

    // print to file
    if (use_file_) {
        file_ << log_line << std::endl;
        file_.flush();
    }
}

/**
 * @brief Set the logging threshold.
 * @param level Minimum log level to print.
 */
void Logger::setThreshold(LogLevel level) {
    threshold_ = level;
}

/**
 * @brief Get the current logging threshold.
 * @return Current threshold level.
 */
Logger::LogLevel Logger::threshold() const {
    return (threshold_);
}

/**
 * @brief Create a LogLine for streaming a message.
 * @param prefix Prefix string for the log line.
 * @param level Log level for the line.
 * @return LogLine object for message streaming.
 */
LogLine Logger::makeLine(const std::string& prefix, LogLevel level) {
    return (LogLine(*this, prefix, level));
}

/**
 * @brief Convert a string to the corresponding Logger::LogLevel.
 * @param levelStr Log level as string (e.g., "DEBUG", "INFO").
 * @return Corresponding LogLevel enum, or INVALID if unrecognized.
 *
 * Case-insensitive conversion. Used to parse log level from user input.
 */
Logger::LogLevel Logger::stringToLevel(const std::string& level_str) {
    std::string level = level_str;
    for (std::string::size_type i = 0; i < level.size(); i++)
        level[i] = std::toupper(static_cast<unsigned char>(level[i]));

    if (level == "NONE")
        return (NONE);
    if (level == "DEBUG")
        return (DEBUG);
    if (level == "INFO")
        return (INFO);
    if (level == "WARNING")
        return (WARNING);
    if (level == "ERROR")
        return (ERROR);
    return (INVALID);
}

/**
 * @brief Ensure a directory exists, create it if missing.
 * @param dir Path of the directory to check/create.
 * @return true if the directory exists or was created successfully.
 */
static bool ensureLogDirectory(const std::string& dir) {
    // this struct is used to get information about a file or
    // directory, like its type, size, permissions, and timestamps.
    struct stat dir_info;
    // stat checks if dir exists and fills the dir_info struct with info
    // about it, returns 0 if path exists
    if (stat(dir.c_str(), &dir_info) == 0) {
        // use macro S_IFDIR to check if it's a directory
        if (dir_info.st_mode & S_IFDIR)
            return (true);
        else
            return (false);
    }
    // create directory
    if (mkdir(dir.c_str(), 0755) == 0)
        return (true);
    return (false);
}

/**
 * @brief Set the log file and ensure Logs directory exists.
 * @param filename Name of the log file to open inside Logs/.
 *
 * Creates the Logs directory if needed. Opens file in truncate mode.
 * If successful, future logs will also be written to this file.
 */
void Logger::setLogFile(const std::string& filename) {
    std::string dir = "Logs";

    if (!ensureLogDirectory(dir)) {
        std::cerr << "Failed to create Logs directory\n";
        return;
    }
    std::string full_path = dir + "/" + filename;

    file_.open(full_path.c_str(), std::ios::out | std::ios::trunc);
    if (file_.is_open())
        use_file_ = true;
    else
        std::cerr << "Failed to open log file\n";
}

/*****************************************************************************
 *                                  LOGLINE                                  *
 *****************************************************************************/

/**
 * @brief Construct a LogLine object for streaming.
 * @param logger Logger instance to flush through.
 * @param prefix Prefix string for this line.
 * @param level Log level of this line.
 */
LogLine::LogLine(Logger& logger, const std::string& prefix,
                 Logger::LogLevel level)
    : logger_(logger), prefix_(prefix), level_(level) {
}

LogLine::LogLine(const LogLine& other)
    : logger_(other.logger_), prefix_(other.prefix_), level_(other.level_) {
    stream_ << other.stream_.str();
}

/**
 * @brief Destructor. Flushes the line to the logger.
 */
LogLine::~LogLine() {
    logger_.flush(prefix_, stream_.str(), level_);
}

/**
 * @brief Initialize the logger with the specified log level.
 * @param levelStr Log level as a string (e.g., "DEBUG", "INFO").
 * @return true if the log level is valid and threshold set, false otherwise.
 *
 * Converts the string to Logger::LogLevel. If invalid, prints usage info.
 */
bool initLogger(const std::string& level_str) {
    Logger::LogLevel level = Logger::stringToLevel(level_str);
    if (level == Logger::INVALID) {
        std::cout << red("Error. invalid log level\n");
        std::cout << cyn("log levels: debug info warning error\n");
        return (false);
    }
    Logger::get().setThreshold(level);
    return (true);
}
