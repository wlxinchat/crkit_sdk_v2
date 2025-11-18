/**
 * @file logger.h
 * @brief 增强的标准化日志管理系统
 * @version 2.0
 * @author wlxinchat@gmail.com
 *
 * 特性:
 * - 分级日志 (DEBUG, INFO, WARN, ERROR, FATAL)
 * - 模块化管理 (可按模块过滤)
 * - 线程安全
 * - 彩色输出 (控制台)
 * - 文件输出
 * - 高性能
 */

#ifndef CRKIT_UTILS_LOGGER_H
#define CRKIT_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <unordered_set>

namespace crkit {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG = 0,    ///< 调试信息
    INFO = 1,     ///< 一般信息
    WARN = 2,     ///< 警告信息
    ERROR = 3,    ///< 错误信息
    FATAL = 4     ///< 致命错误
};

/**
 * @brief 日志模块枚举
 */
enum class LogModule {
    CORE,         ///< 核心模块
    API,          ///< API接口
    BACKEND,      ///< 推理后端
    TENSORRT,     ///< TensorRT后端
    ONNXRUNTIME,  ///< ONNX Runtime后端
    PREPROCESSOR, ///< 预处理器
    POSTPROCESSOR,///< 后处理器
    MODEL,        ///< 模型管理
    MEMORY,       ///< 内存管理
    UTILS,        ///< 工具类
    TEST,         ///< 测试模块
    UNKNOWN       ///< 未知模块
};

/**
 * @brief 日志配置结构
 */
struct LogConfig {
    LogLevel level = LogLevel::INFO;           ///< 最小日志级别
    bool enable_console = true;                ///< 启用控制台输出
    bool enable_file = false;                  ///< 启用文件输出
    bool enable_color = true;                  ///< 启用彩色输出
    bool show_timestamp = true;                ///< 显示时间戳
    bool show_level = true;                    ///< 显示日志级别
    bool show_module = true;                   ///< 显示模块名称
    bool show_location = false;                ///< 显示文件位置
    bool show_thread_id = false;               ///< 显示线程ID
    std::string log_file_path;                 ///< 日志文件路径
    size_t max_file_size = 100 * 1024 * 1024; ///< 最大文件大小(100MB)

    LogConfig() = default;
};

/**
 * @brief 日志管理器类
 *
 * 单例模式，线程安全的日志管理系统
 */
class Logger {
public:
    /**
     * @brief 获取Logger单例
     */
    static Logger& instance();

    /**
     * @brief 设置日志级别
     */
    void setLevel(LogLevel level);

    /**
     * @brief 从字符串设置日志级别
     */
    void setLevel(const std::string& level_str);

    /**
     * @brief 设置日志配置
     */
    void setConfig(const LogConfig& config);

    /**
     * @brief 获取当前配置
     */
    LogConfig getConfig() const;

    /**
     * @brief 设置日志文件
     */
    void setLogFile(const std::string& filepath);

    /**
     * @brief 关闭日志文件
     */
    void closeLogFile();

    /**
     * @brief 启用/禁用模块日志
     */
    void enableModule(LogModule module);
    void disableModule(LogModule module);
    void setModuleFilter(const std::unordered_set<LogModule>& modules);
    void clearModuleFilter();

    /**
     * @brief 启用/禁用彩色输出
     */
    void setColorOutput(bool enable);

    /**
     * @brief 刷新日志缓冲
     */
    void flush();

    /**
     * @brief 核心日志函数
     */
    void log(LogLevel level, LogModule module, const char* file, int line,
             const char* format, ...);

    /**
     * @brief 简化的日志函数（无模块）
     */
    void log(LogLevel level, const char* file, int line,
             const char* format, ...);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 辅助函数
    const char* levelToString(LogLevel level) const;
    const char* moduleToString(LogModule module) const;
    std::string getColorCode(LogLevel level) const;
    std::string getResetCode() const;
    std::string getCurrentTime() const;
    std::string getThreadId() const;
    bool shouldLog(LogLevel level, LogModule module) const;
    void rotateLogFile();
    void writeToConsole(const std::string& message, LogLevel level);
    void writeToFile(const std::string& message);

    // 成员变量
    LogConfig config_;
    std::ofstream log_file_;
    mutable std::mutex mutex_;
    std::unordered_set<LogModule> disabled_modules_;
    bool use_module_filter_;
    size_t current_file_size_;
};

// ============================================================================
// 日志宏定义
// ============================================================================

#ifdef CRKIT_ENABLE_LOGGING

// 带模块的日志宏
#define LOG_MODULE_DEBUG(module, fmt, ...) \
    crkit::Logger::instance().log(crkit::LogLevel::DEBUG, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_MODULE_INFO(module, fmt, ...) \
    crkit::Logger::instance().log(crkit::LogLevel::INFO, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_MODULE_WARN(module, fmt, ...) \
    crkit::Logger::instance().log(crkit::LogLevel::WARN, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_MODULE_ERROR(module, fmt, ...) \
    crkit::Logger::instance().log(crkit::LogLevel::ERROR, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_MODULE_FATAL(module, fmt, ...) \
    crkit::Logger::instance().log(crkit::LogLevel::FATAL, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 通用日志宏（使用UNKNOWN模块）
#define LOG_DEBUG(fmt, ...) \
    LOG_MODULE_DEBUG(crkit::LogModule::UNKNOWN, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    LOG_MODULE_INFO(crkit::LogModule::UNKNOWN, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    LOG_MODULE_WARN(crkit::LogModule::UNKNOWN, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    LOG_MODULE_ERROR(crkit::LogModule::UNKNOWN, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    LOG_MODULE_FATAL(crkit::LogModule::UNKNOWN, fmt, ##__VA_ARGS__)

// 快捷宏 - 自动推断模块
#define LOG_CORE_DEBUG(fmt, ...)        LOG_MODULE_DEBUG(crkit::LogModule::CORE, fmt, ##__VA_ARGS__)
#define LOG_CORE_INFO(fmt, ...)         LOG_MODULE_INFO(crkit::LogModule::CORE, fmt, ##__VA_ARGS__)
#define LOG_CORE_WARN(fmt, ...)         LOG_MODULE_WARN(crkit::LogModule::CORE, fmt, ##__VA_ARGS__)
#define LOG_CORE_ERROR(fmt, ...)        LOG_MODULE_ERROR(crkit::LogModule::CORE, fmt, ##__VA_ARGS__)

#define LOG_API_DEBUG(fmt, ...)         LOG_MODULE_DEBUG(crkit::LogModule::API, fmt, ##__VA_ARGS__)
#define LOG_API_INFO(fmt, ...)          LOG_MODULE_INFO(crkit::LogModule::API, fmt, ##__VA_ARGS__)
#define LOG_API_WARN(fmt, ...)          LOG_MODULE_WARN(crkit::LogModule::API, fmt, ##__VA_ARGS__)
#define LOG_API_ERROR(fmt, ...)         LOG_MODULE_ERROR(crkit::LogModule::API, fmt, ##__VA_ARGS__)

#define LOG_BACKEND_DEBUG(fmt, ...)     LOG_MODULE_DEBUG(crkit::LogModule::BACKEND, fmt, ##__VA_ARGS__)
#define LOG_BACKEND_INFO(fmt, ...)      LOG_MODULE_INFO(crkit::LogModule::BACKEND, fmt, ##__VA_ARGS__)
#define LOG_BACKEND_WARN(fmt, ...)      LOG_MODULE_WARN(crkit::LogModule::BACKEND, fmt, ##__VA_ARGS__)
#define LOG_BACKEND_ERROR(fmt, ...)     LOG_MODULE_ERROR(crkit::LogModule::BACKEND, fmt, ##__VA_ARGS__)

#define LOG_TENSORRT_DEBUG(fmt, ...)    LOG_MODULE_DEBUG(crkit::LogModule::TENSORRT, fmt, ##__VA_ARGS__)
#define LOG_TENSORRT_INFO(fmt, ...)     LOG_MODULE_INFO(crkit::LogModule::TENSORRT, fmt, ##__VA_ARGS__)
#define LOG_TENSORRT_WARN(fmt, ...)     LOG_MODULE_WARN(crkit::LogModule::TENSORRT, fmt, ##__VA_ARGS__)
#define LOG_TENSORRT_ERROR(fmt, ...)    LOG_MODULE_ERROR(crkit::LogModule::TENSORRT, fmt, ##__VA_ARGS__)

#define LOG_PREPROCESS_DEBUG(fmt, ...)  LOG_MODULE_DEBUG(crkit::LogModule::PREPROCESSOR, fmt, ##__VA_ARGS__)
#define LOG_PREPROCESS_INFO(fmt, ...)   LOG_MODULE_INFO(crkit::LogModule::PREPROCESSOR, fmt, ##__VA_ARGS__)
#define LOG_PREPROCESS_WARN(fmt, ...)   LOG_MODULE_WARN(crkit::LogModule::PREPROCESSOR, fmt, ##__VA_ARGS__)
#define LOG_PREPROCESS_ERROR(fmt, ...)  LOG_MODULE_ERROR(crkit::LogModule::PREPROCESSOR, fmt, ##__VA_ARGS__)

#else
    // 日志禁用时的空宏
    #define LOG_MODULE_DEBUG(module, fmt, ...)
    #define LOG_MODULE_INFO(module, fmt, ...)
    #define LOG_MODULE_WARN(module, fmt, ...)
    #define LOG_MODULE_ERROR(module, fmt, ...)
    #define LOG_MODULE_FATAL(module, fmt, ...)

    #define LOG_DEBUG(fmt, ...)
    #define LOG_INFO(fmt, ...)
    #define LOG_WARN(fmt, ...)
    #define LOG_ERROR(fmt, ...)
    #define LOG_FATAL(fmt, ...)

    #define LOG_CORE_DEBUG(fmt, ...)
    #define LOG_CORE_INFO(fmt, ...)
    #define LOG_CORE_WARN(fmt, ...)
    #define LOG_CORE_ERROR(fmt, ...)

    #define LOG_API_DEBUG(fmt, ...)
    #define LOG_API_INFO(fmt, ...)
    #define LOG_API_WARN(fmt, ...)
    #define LOG_API_ERROR(fmt, ...)

    #define LOG_BACKEND_DEBUG(fmt, ...)
    #define LOG_BACKEND_INFO(fmt, ...)
    #define LOG_BACKEND_WARN(fmt, ...)
    #define LOG_BACKEND_ERROR(fmt, ...)

    #define LOG_TENSORRT_DEBUG(fmt, ...)
    #define LOG_TENSORRT_INFO(fmt, ...)
    #define LOG_TENSORRT_WARN(fmt, ...)
    #define LOG_TENSORRT_ERROR(fmt, ...)

    #define LOG_PREPROCESS_DEBUG(fmt, ...)
    #define LOG_PREPROCESS_INFO(fmt, ...)
    #define LOG_PREPROCESS_WARN(fmt, ...)
    #define LOG_PREPROCESS_ERROR(fmt, ...)
#endif

} // namespace crkit

#endif // CRKIT_UTILS_LOGGER_H
