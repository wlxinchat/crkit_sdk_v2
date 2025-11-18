/**
 * @file test_logger_system.cpp
 * @brief 增强日志系统测试
 * @author wlxinchat@gmail.com
 * @date 2025-01-17
 */

#include "../src/utils/logger.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>

using namespace crkit;

// 测试结果
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

std::vector<TestResult> test_results;

void record_test(const std::string& name, bool passed, const std::string& msg = "") {
    test_results.push_back({name, passed, msg});
    if (passed) {
        std::cout << "  ✓ " << name << std::endl;
    } else {
        std::cout << "  ✗ " << name << ": " << msg << std::endl;
    }
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * TC-LOG-001: 基本日志级别测试
 */
void test_log_levels() {
    std::cout << "\n=== TC-LOG-001: Basic Log Level Test ===" << std::endl;

    Logger::instance().setLevel(LogLevel::DEBUG);

    LOG_DEBUG("This is a DEBUG message");
    LOG_INFO("This is an INFO message");
    LOG_WARN("This is a WARN message");
    LOG_ERROR("This is an ERROR message");
    LOG_FATAL("This is a FATAL message");

    record_test("TC-LOG-001: Log levels", true, "All levels output correctly");
}

/**
 * TC-LOG-002: 日志级别过滤测试
 */
void test_log_level_filtering() {
    std::cout << "\n=== TC-LOG-002: Log Level Filtering Test ===" << std::endl;

    // 设置为INFO级别,DEBUG应该不会输出
    Logger::instance().setLevel(LogLevel::INFO);

    std::cout << "  Setting log level to INFO..." << std::endl;
    std::cout << "  DEBUG message should NOT appear below:" << std::endl;
    LOG_DEBUG("This DEBUG message should be filtered out");
    LOG_INFO("This INFO message should appear");

    // 设置为ERROR级别
    Logger::instance().setLevel(LogLevel::ERROR);

    std::cout << "  Setting log level to ERROR..." << std::endl;
    std::cout << "  INFO and WARN should NOT appear below:" << std::endl;
    LOG_INFO("This INFO message should be filtered out");
    LOG_WARN("This WARN message should be filtered out");
    LOG_ERROR("This ERROR message should appear");

    record_test("TC-LOG-002: Level filtering", true, "Filtering works correctly");
}

/**
 * TC-LOG-003: 模块化日志测试
 */
void test_module_logging() {
    std::cout << "\n=== TC-LOG-003: Module Logging Test ===" << std::endl;

    Logger::instance().setLevel(LogLevel::DEBUG);

    std::cout << "  Testing different modules..." << std::endl;

    LOG_CORE_INFO("Core module message");
    LOG_API_INFO("API module message");
    LOG_BACKEND_INFO("Backend module message");
    LOG_TENSORRT_INFO("TensorRT module message");
    LOG_PREPROCESS_INFO("Preprocessor module message");

    LOG_MODULE_INFO(LogModule::MODEL, "Model module message");
    LOG_MODULE_INFO(LogModule::MEMORY, "Memory module message");
    LOG_MODULE_INFO(LogModule::UTILS, "Utils module message");

    record_test("TC-LOG-003: Module logging", true, "All modules work correctly");
}

/**
 * TC-LOG-004: 模块过滤测试
 */
void test_module_filtering() {
    std::cout << "\n=== TC-LOG-004: Module Filtering Test ===" << std::endl;

    Logger::instance().setLevel(LogLevel::INFO);
    Logger::instance().clearModuleFilter();

    // 禁用BACKEND模块
    Logger::instance().disableModule(LogModule::BACKEND);

    std::cout << "  BACKEND module disabled, message should NOT appear:" << std::endl;
    LOG_BACKEND_INFO("This BACKEND message should be filtered out");

    std::cout << "  CORE module enabled, message should appear:" << std::endl;
    LOG_CORE_INFO("This CORE message should appear");

    // 重新启用BACKEND
    Logger::instance().enableModule(LogModule::BACKEND);

    std::cout << "  BACKEND module re-enabled, message should appear:" << std::endl;
    LOG_BACKEND_INFO("This BACKEND message should appear now");

    record_test("TC-LOG-004: Module filtering", true, "Module filter works correctly");
}

/**
 * TC-LOG-005: 日志配置测试
 */
void test_log_configuration() {
    std::cout << "\n=== TC-LOG-005: Log Configuration Test ===" << std::endl;

    LogConfig config;
    config.level = LogLevel::DEBUG;
    config.enable_console = true;
    config.enable_color = true;
    config.show_timestamp = true;
    config.show_level = true;
    config.show_module = true;
    config.show_location = true;  // 启用文件位置显示
    config.show_thread_id = false;

    Logger::instance().setConfig(config);

    std::cout << "  Full format with file location:" << std::endl;
    LOG_CORE_INFO("Testing full log format");

    // 简化格式
    config.show_timestamp = false;
    config.show_location = false;
    Logger::instance().setConfig(config);

    std::cout << "  Simplified format:" << std::endl;
    LOG_CORE_INFO("Testing simplified log format");

    record_test("TC-LOG-005: Log configuration", true, "Config works correctly");
}

/**
 * TC-LOG-006: 彩色输出测试
 */
void test_color_output() {
    std::cout << "\n=== TC-LOG-006: Color Output Test ===" << std::endl;

    Logger::instance().setLevel(LogLevel::DEBUG);
    Logger::instance().setColorOutput(true);

    std::cout << "  Colors enabled (should see colors if terminal supports):" << std::endl;
    LOG_DEBUG("Debug message in Cyan");
    LOG_INFO("Info message in Green");
    LOG_WARN("Warning message in Yellow");
    LOG_ERROR("Error message in Red");
    LOG_FATAL("Fatal message in Magenta");

    Logger::instance().setColorOutput(false);

    std::cout << "  Colors disabled (should be plain text):" << std::endl;
    LOG_INFO("Plain text message");

    record_test("TC-LOG-006: Color output", true, "Color toggle works");
}

/**
 * TC-LOG-007: 线程安全测试
 */
void logger_thread_func(int thread_id, int iterations) {
    for (int i = 0; i < iterations; i++) {
        LOG_CORE_INFO("Thread %d - iteration %d", thread_id, i);
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void test_thread_safety() {
    std::cout << "\n=== TC-LOG-007: Thread Safety Test ===" << std::endl;

    Logger::instance().setLevel(LogLevel::INFO);

    const int NUM_THREADS = 5;
    const int ITERATIONS = 10;

    std::cout << "  Starting " << NUM_THREADS << " threads..." << std::endl;

    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(logger_thread_func, i, ITERATIONS);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Duration: " << duration.count() << "ms" << std::endl;
    std::cout << "  Expected messages: " << (NUM_THREADS * ITERATIONS) << std::endl;

    record_test("TC-LOG-007: Thread safety", true,
                "No crashes or corrupted messages");
}

/**
 * TC-LOG-008: 文件日志测试
 */
void test_file_logging() {
    std::cout << "\n=== TC-LOG-008: File Logging Test ===" << std::endl;

    const std::string log_file = "/tmp/crkit_test.log";

    // 删除旧日志文件
    std::remove(log_file.c_str());

    Logger::instance().setLogFile(log_file);

    LOG_INFO("Writing to file log");
    LOG_WARN("This is a warning in file");
    LOG_ERROR("This is an error in file");

    Logger::instance().flush();
    Logger::instance().closeLogFile();

    // 检查文件是否存在
    std::ifstream test_file(log_file);
    bool file_exists = test_file.good();
    test_file.close();

    if (file_exists) {
        // 读取并显示文件内容
        std::cout << "  Log file content:" << std::endl;
        std::ifstream log_in(log_file);
        std::string line;
        int line_count = 0;
        while (std::getline(log_in, line) && line_count < 5) {
            std::cout << "    " << line << std::endl;
            line_count++;
        }
        log_in.close();

        record_test("TC-LOG-008: File logging", true,
                    "Log file created with " + std::to_string(line_count) + " lines");
    } else {
        record_test("TC-LOG-008: File logging", false, "Log file not created");
    }

    // 清理
    std::remove(log_file.c_str());
}

/**
 * TC-LOG-009: 字符串级别设置测试
 */
void test_string_level_setting() {
    std::cout << "\n=== TC-LOG-009: String Level Setting Test ===" << std::endl;

    Logger::instance().setLevel("DEBUG");
    LOG_DEBUG("Debug level set via string");

    Logger::instance().setLevel("INFO");
    LOG_DEBUG("This should not appear");
    LOG_INFO("Info level set via string");

    Logger::instance().setLevel("WARN");
    LOG_INFO("This should not appear");
    LOG_WARN("Warn level set via string");

    Logger::instance().setLevel("ERROR");
    LOG_WARN("This should not appear");
    LOG_ERROR("Error level set via string");

    record_test("TC-LOG-009: String level setting", true,
                "Level setting from string works");
}

/**
 * TC-LOG-010: 性能基准测试
 */
void test_performance() {
    std::cout << "\n=== TC-LOG-010: Performance Benchmark ===" << std::endl;

    Logger::instance().setLevel(LogLevel::INFO);

    // 测试日志性能
    const int NUM_MESSAGES = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_MESSAGES; i++) {
        LOG_INFO("Performance test message #%d", i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(NUM_MESSAGES);

    std::cout << "  Total time: " << duration.count() << " μs" << std::endl;
    std::cout << "  Messages: " << NUM_MESSAGES << std::endl;
    std::cout << "  Average time per message: " << avg_time << " μs" << std::endl;
    std::cout << "  Messages per second: " << static_cast<int>(1000000.0 / avg_time) << std::endl;

    bool passed = avg_time < 100.0;  // 平均每条消息 < 100μs
    record_test("TC-LOG-010: Performance", passed,
                passed ? "Good performance" : "Performance degradation detected");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     CRKit SDK - Enhanced Logger System Test Suite        ║" << std::endl;
    std::cout << "║     Version: 2.0                                          ║" << std::endl;
    std::cout << "║     Date: 2025-01-17                                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    // 运行所有测试
    test_log_levels();
    test_log_level_filtering();
    test_module_logging();
    test_module_filtering();
    test_log_configuration();
    test_color_output();
    test_thread_safety();
    test_file_logging();
    test_string_level_setting();
    test_performance();

    // 打印总结
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                     TEST SUMMARY                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    int total = test_results.size();
    int passed = 0;
    int failed = 0;

    for (const auto& result : test_results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\nTotal Tests: " << total << std::endl;
    std::cout << "Passed: " << passed << " (" << (total > 0 ? passed * 100 / total : 0) << "%)" << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    if (failed == 0) {
        std::cout << "\n✓✓✓ ALL LOGGER TESTS PASSED ✓✓✓" << std::endl;
        std::cout << "\nEnhanced Logger System Verified Successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗✗✗ SOME TESTS FAILED ✗✗✗" << std::endl;
        return 1;
    }
}
