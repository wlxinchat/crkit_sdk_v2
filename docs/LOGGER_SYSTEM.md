# CRKit SDK - 日志系统使用指南

**版本**: 2.0
**作者**: wlxinchat@gmail.com
**日期**: 2025-01-17

---

## 概述

CRKit SDK v1.0.0包含一个全面增强的标准化日志管理系统，提供分级、模块化的日志记录能力。

### 主要特性

✅ **分级日志**: DEBUG, INFO, WARN, ERROR, FATAL
✅ **模块化管理**: 可按模块精确过滤日志
✅ **线程安全**: 完全的多线程安全
✅ **彩色输出**: 终端彩色显示（可配置）
✅ **文件日志**: 支持日志文件输出和轮转
✅ **高性能**: 每秒可处理30万+条日志
✅ **灵活配置**: 丰富的配置选项

---

## 快速开始

### C API使用

```c
#include "crkit_api.h"

int main() {
    // 设置日志级别
    crkit_set_log_level("DEBUG");

    // 设置日志文件（可选）
    crkit_set_log_file("/var/log/crkit.log");

    // 正常使用SDK
    // ...

    return 0;
}
```

### C++ 内部使用

```cpp
#include "utils/logger.h"
using namespace crkit;

void example_function() {
    // 通用日志
    LOG_DEBUG("Debug information");
    LOG_INFO("Application started");
    LOG_WARN("Resource usage high: %d%%", usage);
    LOG_ERROR("Failed to load config");
    LOG_FATAL("Critical error occurred");

    // 模块化日志
    LOG_CORE_INFO("Core initialized");
    LOG_API_INFO("API version: %s", version);
    LOG_BACKEND_INFO("Using TensorRT backend");
    LOG_TENSORRT_INFO("TensorRT engine created");
    LOG_PREPROCESS_INFO("Image preprocessed: %dx%d", w, h);
}
```

---

## 日志级别

### 级别定义

| 级别 | 值 | 用途 | 颜色 |
|------|---|------|------|
| DEBUG | 0 | 详细调试信息 | 青色 |
| INFO | 1 | 一般信息 | 绿色 |
| WARN | 2 | 警告信息 | 黄色 |
| ERROR | 3 | 错误信息 | 红色 |
| FATAL | 4 | 致命错误 | 品红 |

### 级别过滤

设置日志级别后，只有等于或高于该级别的日志才会输出：

```cpp
Logger::instance().setLevel(LogLevel::WARN);

LOG_DEBUG("Not shown");  // 不会输出
LOG_INFO("Not shown");   // 不会输出
LOG_WARN("Shown");       // 会输出
LOG_ERROR("Shown");      // 会输出
LOG_FATAL("Shown");      // 会输出
```

---

## 模块化日志

### 可用模块

```cpp
enum class LogModule {
    CORE,         // 核心模块
    API,          // API接口
    BACKEND,      // 推理后端
    TENSORRT,     // TensorRT后端
    ONNXRUNTIME,  // ONNX Runtime后端
    PREPROCESSOR, // 预处理器
    POSTPROCESSOR,// 后处理器
    MODEL,        // 模型管理
    MEMORY,       // 内存管理
    UTILS,        // 工具类
    TEST,         // 测试模块
    UNKNOWN       // 未知模块
};
```

### 使用模块日志

**方式1: 使用快捷宏**

```cpp
LOG_CORE_INFO("Core module initialized");
LOG_API_DEBUG("API called: %s", api_name);
LOG_BACKEND_WARN("Backend latency high: %dms", latency);
LOG_TENSORRT_ERROR("TensorRT error: %s", error_msg);
LOG_PREPROCESS_INFO("Preprocessed image: %dx%d", w, h);
```

**方式2: 使用通用宏**

```cpp
LOG_MODULE_INFO(LogModule::MODEL, "Model loaded: %s", model_path);
LOG_MODULE_ERROR(LogModule::MEMORY, "Out of memory: %zu bytes", size);
```

### 模块过滤

```cpp
// 禁用特定模块
Logger::instance().disableModule(LogModule::DEBUG);
Logger::instance().disableModule(LogModule::TENSORRT);

// 重新启用
Logger::instance().enableModule(LogModule::TENSORRT);

// 清除所有过滤
Logger::instance().clearModuleFilter();
```

---

## 日志配置

### 配置结构

```cpp
struct LogConfig {
    LogLevel level = LogLevel::INFO;           // 最小日志级别
    bool enable_console = true;                // 启用控制台输出
    bool enable_file = false;                  // 启用文件输出
    bool enable_color = true;                  // 启用彩色输出
    bool show_timestamp = true;                // 显示时间戳
    bool show_level = true;                    // 显示日志级别
    bool show_module = true;                   // 显示模块名称
    bool show_location = false;                // 显示文件位置
    bool show_thread_id = false;               // 显示线程ID
    std::string log_file_path;                 // 日志文件路径
    size_t max_file_size = 100 * 1024 * 1024; // 最大文件大小(100MB)
};
```

### 应用配置

```cpp
LogConfig config;
config.level = LogLevel::DEBUG;
config.enable_console = true;
config.enable_file = true;
config.enable_color = true;
config.show_timestamp = true;
config.show_level = true;
config.show_module = true;
config.show_location = true;  // 显示文件名:行号
config.show_thread_id = true; // 显示线程ID
config.log_file_path = "/var/log/crkit.log";
config.max_file_size = 50 * 1024 * 1024; // 50MB

Logger::instance().setConfig(config);
```

---

## 日志输出格式

### 完整格式

```
2025-01-17 12:34:56 [INFO] [CORE] [thread:12345] Message text (file.cpp:123)
```

组成部分：
- `2025-01-17 12:34:56`: 时间戳（可选）
- `[INFO]`: 日志级别（可选）
- `[CORE]`: 模块名称（可选）
- `[thread:12345]`: 线程ID（可选）
- `Message text`: 日志消息
- `(file.cpp:123)`: 文件位置（可选）

### 简化格式

```cpp
LogConfig config;
config.show_timestamp = false;
config.show_location = false;
config.show_thread_id = false;
Logger::instance().setConfig(config);
```

输出：
```
[INFO] [CORE] Message text
```

---

## 文件日志

### 启用文件日志

**方法1: 通过C API**

```c
crkit_set_log_file("/var/log/crkit.log");
```

**方法2: 通过C++接口**

```cpp
Logger::instance().setLogFile("/var/log/crkit.log");
```

**方法3: 通过配置**

```cpp
LogConfig config;
config.enable_file = true;
config.log_file_path = "/var/log/crkit.log";
config.max_file_size = 100 * 1024 * 1024; // 100MB
Logger::instance().setConfig(config);
```

### 日志轮转

当日志文件达到 `max_file_size` 时，会自动轮转：
- 当前文件重命名为 `.old`
- 创建新的日志文件

```cpp
config.max_file_size = 50 * 1024 * 1024;  // 50MB后轮转
```

### 关闭文件日志

```cpp
Logger::instance().closeLogFile();
// 或
crkit_set_log_file(NULL);
```

---

## 高级用法

### 1. 动态控制日志级别

```cpp
// 运行时改变日志级别
if (debug_mode) {
    Logger::instance().setLevel(LogLevel::DEBUG);
} else {
    Logger::instance().setLevel(LogLevel::INFO);
}
```

### 2. 条件日志

```cpp
#ifdef DEBUG_MODE
    Logger::instance().setLevel(LogLevel::DEBUG);
#else
    Logger::instance().setLevel(LogLevel::INFO);
#endif
```

### 3. 性能敏感代码

在性能敏感的代码中，可以先检查日志级别：

```cpp
if (Logger::instance().getConfig().level <= LogLevel::DEBUG) {
    std::string expensive_debug_info = computeExpensiveDebugInfo();
    LOG_DEBUG("Debug info: %s", expensive_debug_info.c_str());
}
```

### 4. 多线程日志

日志系统完全线程安全，可在多线程环境中直接使用：

```cpp
void worker_thread(int id) {
    LOG_CORE_INFO("Worker %d started", id);
    // 执行任务
    LOG_CORE_INFO("Worker %d finished", id);
}

std::thread t1(worker_thread, 1);
std::thread t2(worker_thread, 2);
t1.join();
t2.join();
```

### 5. 刷新日志缓冲

在关键点强制刷新日志：

```cpp
Logger::instance().flush();
```

---

## 性能特性

### 性能指标

基于测试（TC-LOG-010）：
- **吞吐量**: 304,000+ 消息/秒
- **平均延迟**: 3.3 微秒/消息
- **线程安全**: 无性能损失

### 优化建议

1. **避免频繁的DEBUG日志**
   ```cpp
   // 生产环境设置为INFO或更高级别
   Logger::instance().setLevel(LogLevel::INFO);
   ```

2. **使用模块过滤**
   ```cpp
   // 在性能测试时禁用某些模块的日志
   Logger::instance().disableModule(LogModule::DEBUG);
   ```

3. **避免昂贵的字符串格式化**
   ```cpp
   // 不好
   LOG_DEBUG("Data: %s", expensive_to_string_conversion().c_str());

   // 好
   if (Logger::instance().getConfig().level <= LogLevel::DEBUG) {
       LOG_DEBUG("Data: %s", expensive_to_string_conversion().c_str());
   }
   ```

---

## 最佳实践

### 1. 选择合适的日志级别

```cpp
LOG_DEBUG("Variable x = %d", x);              // 详细调试信息
LOG_INFO("Application started successfully"); // 重要事件
LOG_WARN("Retry attempt %d of %d", i, max);   // 潜在问题
LOG_ERROR("Failed to open file: %s", path);   // 错误但可恢复
LOG_FATAL("Out of memory, aborting");         // 致命错误
```

### 2. 使用适当的模块

```cpp
LOG_API_INFO("crkit_create_engine called");
LOG_BACKEND_INFO("Initializing TensorRT backend");
LOG_MODEL_INFO("Loading model from %s", path);
LOG_MEMORY_WARN("Memory usage: %zu MB", usage_mb);
```

### 3. 提供上下文信息

```cpp
// 不好
LOG_ERROR("Load failed");

// 好
LOG_ERROR("Failed to load model from %s: %s", path, error.c_str());
```

### 4. 避免敏感信息

```cpp
// 不要记录密码、密钥等敏感信息
LOG_INFO("User logged in: %s", username);  // OK
LOG_INFO("Password: %s", password);        // 永远不要这样做！
```

---

## 测试验证

运行日志系统测试：

```bash
g++ -std=c++11 -pthread -DCRKIT_ENABLE_LOGGING -I./src \
    -o test_logger tests/test_logger_system.cpp src/utils/logger.cpp

./test_logger
```

预期输出：
```
✓✓✓ ALL LOGGER TESTS PASSED ✓✓✓
Enhanced Logger System Verified Successfully!
```

---

## 故障排除

### 问题1: 日志不输出

**原因**: 日志级别过滤
**解决**:
```cpp
Logger::instance().setLevel(LogLevel::DEBUG);
```

### 问题2: 文件日志无法写入

**原因**: 权限不足或路径不存在
**解决**:
```bash
# 检查目录权限
ls -ld /var/log/
# 或使用用户目录
crkit_set_log_file("/tmp/crkit.log");
```

### 问题3: 性能下降

**原因**: DEBUG级别日志过多
**解决**:
```cpp
// 生产环境使用INFO或更高级别
Logger::instance().setLevel(LogLevel::INFO);

// 或禁用文件日志
Logger::instance().closeLogFile();
```

---

## API参考

### C API

| 函数 | 说明 |
|------|------|
| `crkit_set_log_level(const char* level)` | 设置日志级别 |
| `crkit_set_log_file(const char* path)` | 设置日志文件 |

### C++ API

| 函数 | 说明 |
|------|------|
| `Logger::instance()` | 获取Logger单例 |
| `setLevel(LogLevel)` | 设置日志级别 |
| `setLevel(const string&)` | 从字符串设置级别 |
| `setConfig(const LogConfig&)` | 设置完整配置 |
| `setLogFile(const string&)` | 设置日志文件 |
| `closeLogFile()` | 关闭日志文件 |
| `enableModule(LogModule)` | 启用模块 |
| `disableModule(LogModule)` | 禁用模块 |
| `setColorOutput(bool)` | 设置彩色输出 |
| `flush()` | 刷新缓冲 |

---

## 示例代码

### 完整示例

```cpp
#include "crkit_api.h"
#include "utils/logger.h"

int main() {
    using namespace crkit;

    // 1. 配置日志系统
    LogConfig config;
    config.level = LogLevel::DEBUG;
    config.enable_console = true;
    config.enable_file = true;
    config.log_file_path = "/tmp/crkit_app.log";
    config.enable_color = true;
    Logger::instance().setConfig(config);

    // 2. 记录应用启动
    LOG_CORE_INFO("Application starting...");

    // 3. 创建引擎
    CRKitEngine engine;
    CRKitEngineConfig eng_config;
    crkit_create_default_engine_config(&eng_config);

    LOG_API_INFO("Creating inference engine");
    CRKitStatus status = crkit_create_engine(&engine, &eng_config);

    if (status != CRKIT_SUCCESS) {
        LOG_API_ERROR("Failed to create engine: %s",
                      crkit_get_last_error());
        return 1;
    }

    LOG_API_INFO("Engine created successfully");

    // 4. 加载模型
    CRKitModel model;
    CRKitModelConfig model_config;
    crkit_create_default_model_config(&model_config);
    model_config.model_path = "yolo11n.onnx";

    LOG_BACKEND_INFO("Loading model: %s", model_config.model_path);
    status = crkit_load_model(engine, &model, &model_config);

    if (status != CRKIT_SUCCESS) {
        LOG_BACKEND_ERROR("Model load failed: %s",
                          crkit_get_last_error());
        crkit_destroy_engine(engine);
        return 1;
    }

    LOG_BACKEND_INFO("Model loaded successfully");

    // 5. 推理
    CRKitResult* result;
    LOG_CORE_INFO("Running inference...");
    status = crkit_infer_from_file(model, "test.jpg", &result);

    if (status == CRKIT_SUCCESS) {
        LOG_CORE_INFO("Inference completed: %d detections",
                      result->num_detections);
        crkit_free_result(result);
    } else {
        LOG_CORE_ERROR("Inference failed");
    }

    // 6. 清理
    LOG_CORE_INFO("Cleaning up...");
    crkit_unload_model(model);
    crkit_destroy_engine(engine);

    LOG_CORE_INFO("Application finished");
    Logger::instance().flush();

    return 0;
}
```

---

## 联系方式

**技术支持**: wlxinchat@gmail.com
**问题反馈**: GitHub Issues

---

**文档版本**: 2.0
**最后更新**: 2025-01-17
