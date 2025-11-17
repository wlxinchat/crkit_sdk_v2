/**
 * @file logger.h
 * @brief 日志系统
 */

#ifndef CRKIT_UTILS_LOGGER_H
#define CRKIT_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace crkit {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setLevel(const std::string& level_str);
    void setLogFile(const std::string& filepath);
    void closeLogFile();

    void log(LogLevel level, const char* file, int line,
             const char* format, ...);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel level_;
    std::ofstream log_file_;
    std::mutex mutex_;
    bool use_file_;

    const char* levelToString(LogLevel level);
    std::string getCurrentTime();
};

#ifdef CRKIT_ENABLE_LOGGING
    #define LOG_DEBUG(fmt, ...) \
        crkit::Logger::instance().log(crkit::LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) \
        crkit::Logger::instance().log(crkit::LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) \
        crkit::Logger::instance().log(crkit::LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) \
        crkit::Logger::instance().log(crkit::LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
    #define LOG_INFO(fmt, ...)
    #define LOG_WARN(fmt, ...)
    #define LOG_ERROR(fmt, ...)
#endif

} // namespace crkit

#endif // CRKIT_UTILS_LOGGER_H
