/**
 * @file logger.cpp
 * @brief 日志系统实现
 */

#include "logger.h"
#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <iostream>

namespace crkit {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger()
    : level_(LogLevel::INFO), use_file_(false) {
}

Logger::~Logger() {
    closeLogFile();
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::setLevel(const std::string& level_str) {
    LogLevel level = LogLevel::INFO;
    if (level_str == "DEBUG") {
        level = LogLevel::DEBUG;
    } else if (level_str == "INFO") {
        level = LogLevel::INFO;
    } else if (level_str == "WARN") {
        level = LogLevel::WARN;
    } else if (level_str == "ERROR") {
        level = LogLevel::ERROR;
    }
    setLevel(level);
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    closeLogFile();
    log_file_.open(filepath, std::ios::out | std::ios::app);
    use_file_ = log_file_.is_open();
}

void Logger::closeLogFile() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
    use_file_ = false;
}

void Logger::log(LogLevel level, const char* file, int line,
                 const char* format, ...) {
    if (level < level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::string message = getCurrentTime() + " [" + levelToString(level) + "] "
                        + "[" + file + ":" + std::to_string(line) + "] "
                        + buffer;

    if (use_file_ && log_file_.is_open()) {
        log_file_ << message << std::endl;
        log_file_.flush();
    } else {
        if (level >= LogLevel::ERROR) {
            std::cerr << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

} // namespace crkit
