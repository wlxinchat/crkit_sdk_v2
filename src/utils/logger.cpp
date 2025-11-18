/**
 * @file logger.cpp
 * @brief 增强的标准化日志管理系统实现
 * @version 2.0
 * @author wlxinchat@gmail.com
 */

#include "logger.h"
#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace crkit {

// ============================================================================
// Logger Implementation
// ============================================================================

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger()
    : use_module_filter_(false)
    , current_file_size_(0) {
    // 默认配置
    config_.level = LogLevel::INFO;
    config_.enable_console = true;
    config_.enable_file = false;
    config_.enable_color = true;
    config_.show_timestamp = true;
    config_.show_level = true;
    config_.show_module = true;
    config_.show_location = false;
    config_.show_thread_id = false;
}

Logger::~Logger() {
    closeLogFile();
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.level = level;
}

void Logger::setLevel(const std::string& level_str) {
    LogLevel level = LogLevel::INFO;
    if (level_str == "DEBUG") {
        level = LogLevel::DEBUG;
    } else if (level_str == "INFO") {
        level = LogLevel::INFO;
    } else if (level_str == "WARN" || level_str == "WARNING") {
        level = LogLevel::WARN;
    } else if (level_str == "ERROR") {
        level = LogLevel::ERROR;
    } else if (level_str == "FATAL") {
        level = LogLevel::FATAL;
    }
    setLevel(level);
}

void Logger::setConfig(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    // 如果启用文件日志且路径非空
    if (config_.enable_file && !config_.log_file_path.empty()) {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        log_file_.open(config_.log_file_path, std::ios::out | std::ios::app);
        current_file_size_ = log_file_.tellp();
    }
}

LogConfig Logger::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    closeLogFile();
    log_file_.open(filepath, std::ios::out | std::ios::app);
    if (log_file_.is_open()) {
        config_.enable_file = true;
        config_.log_file_path = filepath;
        current_file_size_ = log_file_.tellp();
    }
}

void Logger::closeLogFile() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
    config_.enable_file = false;
    current_file_size_ = 0;
}

void Logger::enableModule(LogModule module) {
    std::lock_guard<std::mutex> lock(mutex_);
    disabled_modules_.erase(module);
}

void Logger::disableModule(LogModule module) {
    std::lock_guard<std::mutex> lock(mutex_);
    disabled_modules_.insert(module);
}

void Logger::setModuleFilter(const std::unordered_set<LogModule>& modules) {
    std::lock_guard<std::mutex> lock(mutex_);
    disabled_modules_ = modules;
    use_module_filter_ = !modules.empty();
}

void Logger::clearModuleFilter() {
    std::lock_guard<std::mutex> lock(mutex_);
    disabled_modules_.clear();
    use_module_filter_ = false;
}

void Logger::setColorOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enable_color = enable;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

void Logger::log(LogLevel level, LogModule module, const char* file, int line,
                 const char* format, ...) {
    if (!shouldLog(level, module)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // 格式化用户消息
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // 构建完整日志消息
    std::stringstream ss;

    // 时间戳
    if (config_.show_timestamp) {
        ss << getCurrentTime() << " ";
    }

    // 日志级别
    if (config_.show_level) {
        ss << "[" << std::setw(5) << levelToString(level) << "] ";
    }

    // 模块名称
    if (config_.show_module && module != LogModule::UNKNOWN) {
        ss << "[" << std::setw(12) << moduleToString(module) << "] ";
    }

    // 线程ID
    if (config_.show_thread_id) {
        ss << "[" << getThreadId() << "] ";
    }

    // 用户消息
    ss << buffer;

    // 文件位置
    if (config_.show_location) {
        // 提取文件名
        const char* filename = strrchr(file, '/');
        if (!filename) {
            filename = strrchr(file, '\\');
        }
        filename = filename ? filename + 1 : file;

        ss << " (" << filename << ":" << line << ")";
    }

    std::string message = ss.str();

    // 输出到控制台
    if (config_.enable_console) {
        writeToConsole(message, level);
    }

    // 输出到文件
    if (config_.enable_file && log_file_.is_open()) {
        writeToFile(message);
    }
}

void Logger::log(LogLevel level, const char* file, int line,
                 const char* format, ...) {
    if (level < config_.level) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::stringstream ss;

    if (config_.show_timestamp) {
        ss << getCurrentTime() << " ";
    }

    if (config_.show_level) {
        ss << "[" << std::setw(5) << levelToString(level) << "] ";
    }

    if (config_.show_thread_id) {
        ss << "[" << getThreadId() << "] ";
    }

    ss << buffer;

    if (config_.show_location) {
        const char* filename = strrchr(file, '/');
        if (!filename) {
            filename = strrchr(file, '\\');
        }
        filename = filename ? filename + 1 : file;
        ss << " (" << filename << ":" << line << ")";
    }

    std::string message = ss.str();

    if (config_.enable_console) {
        writeToConsole(message, level);
    }

    if (config_.enable_file && log_file_.is_open()) {
        writeToFile(message);
    }
}

// ============================================================================
// Private Helper Functions
// ============================================================================

const char* Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

const char* Logger::moduleToString(LogModule module) const {
    switch (module) {
        case LogModule::CORE:         return "CORE";
        case LogModule::API:          return "API";
        case LogModule::BACKEND:      return "BACKEND";
        case LogModule::TENSORRT:     return "TENSORRT";
        case LogModule::ONNXRUNTIME:  return "ONNXRUNTIME";
        case LogModule::PREPROCESSOR: return "PREPROCESSOR";
        case LogModule::POSTPROCESSOR:return "POSTPROCESSOR";
        case LogModule::MODEL:        return "MODEL";
        case LogModule::MEMORY:       return "MEMORY";
        case LogModule::UTILS:        return "UTILS";
        case LogModule::TEST:         return "TEST";
        case LogModule::UNKNOWN:      return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

std::string Logger::getColorCode(LogLevel level) const {
    if (!config_.enable_color) {
        return "";
    }

#ifdef _WIN32
    // Windows console color codes (not used for now)
    return "";
#else
    // ANSI color codes for Unix/Linux
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m";    // Cyan
        case LogLevel::INFO:  return "\033[32m";    // Green
        case LogLevel::WARN:  return "\033[33m";    // Yellow
        case LogLevel::ERROR: return "\033[31m";    // Red
        case LogLevel::FATAL: return "\033[35m";    // Magenta
        default: return "";
    }
#endif
}

std::string Logger::getResetCode() const {
    if (!config_.enable_color) {
        return "";
    }
#ifdef _WIN32
    return "";
#else
    return "\033[0m";
#endif
}

std::string Logger::getCurrentTime() const {
    time_t now = time(nullptr);
    struct tm tm_buf;
    char buf[64];

    #ifdef _WIN32
    localtime_s(&tm_buf, &now);
    #else
    localtime_r(&now, &tm_buf);
    #endif

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

std::string Logger::getThreadId() const {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

bool Logger::shouldLog(LogLevel level, LogModule module) const {
    // 检查日志级别
    if (level < config_.level) {
        return false;
    }

    // 检查模块过滤
    if (!disabled_modules_.empty()) {
        if (disabled_modules_.find(module) != disabled_modules_.end()) {
            return false;
        }
    }

    return true;
}

void Logger::rotateLogFile() {
    if (!log_file_.is_open() || current_file_size_ < config_.max_file_size) {
        return;
    }

    log_file_.close();

    // 重命名旧文件
    std::string old_path = config_.log_file_path;
    std::string new_path = old_path + ".old";

    // 移动文件
    std::rename(old_path.c_str(), new_path.c_str());

    // 创建新文件
    log_file_.open(old_path, std::ios::out | std::ios::app);
    current_file_size_ = 0;
}

void Logger::writeToConsole(const std::string& message, LogLevel level) {
    std::string colored_message = getColorCode(level) + message + getResetCode();

    if (level >= LogLevel::ERROR) {
        std::cerr << colored_message << std::endl;
    } else {
        std::cout << colored_message << std::endl;
    }
}

void Logger::writeToFile(const std::string& message) {
    log_file_ << message << std::endl;
    current_file_size_ += message.length() + 1;

    // 检查是否需要rotate
    if (current_file_size_ >= config_.max_file_size) {
        rotateLogFile();
    }
}

} // namespace crkit
