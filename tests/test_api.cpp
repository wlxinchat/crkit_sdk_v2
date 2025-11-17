/**
 * @file test_api.cpp
 * @brief API功能测试
 */

#include "crkit_api.h"
#include <iostream>
#include <cassert>
#include <cstring>

void test_version() {
    std::cout << "Test: Version Info" << std::endl;
    const char* version = crkit_get_version();
    assert(version != nullptr);
    assert(std::strlen(version) > 0);
    std::cout << "  Version: " << version << std::endl;
    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_config_creation() {
    std::cout << "Test: Config Creation" << std::endl;

    // 测试引擎配置
    CRKitEngineConfig engine_config;
    CRKitStatus status = crkit_create_default_engine_config(&engine_config);
    assert(status == CRKIT_SUCCESS);
    assert(engine_config.max_batch_size > 0);
    assert(engine_config.num_worker_threads > 0);
    std::cout << "  Default batch size: " << engine_config.max_batch_size << std::endl;
    std::cout << "  Default threads: " << engine_config.num_worker_threads << std::endl;

    // 测试模型配置
    CRKitModelConfig model_config;
    status = crkit_create_default_model_config(&model_config);
    assert(status == CRKIT_SUCCESS);
    assert(model_config.input_width == 640);
    assert(model_config.input_height == 640);
    assert(model_config.conf_threshold > 0.0f && model_config.conf_threshold < 1.0f);
    std::cout << "  Default input size: " << model_config.input_width << "x"
              << model_config.input_height << std::endl;
    std::cout << "  Default conf threshold: " << model_config.conf_threshold << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_error_handling() {
    std::cout << "Test: Error Handling" << std::endl;

    // 测试空指针错误
    CRKitStatus status = crkit_create_default_engine_config(nullptr);
    assert(status != CRKIT_SUCCESS);

    const char* error = crkit_get_last_error();
    assert(error != nullptr);
    std::cout << "  Error message: " << error << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_log_control() {
    std::cout << "Test: Log Control" << std::endl;

    CRKitStatus status;

    // 设置日志级别
    status = crkit_set_log_level("DEBUG");
    assert(status == CRKIT_SUCCESS);

    status = crkit_set_log_level("INFO");
    assert(status == CRKIT_SUCCESS);

    status = crkit_set_log_level("WARN");
    assert(status == CRKIT_SUCCESS);

    status = crkit_set_log_level("ERROR");
    assert(status == CRKIT_SUCCESS);

    // 恢复INFO级别
    status = crkit_set_log_level("INFO");
    assert(status == CRKIT_SUCCESS);

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_engine_lifecycle() {
    std::cout << "Test: Engine Lifecycle" << std::endl;

    CRKitEngineConfig config;
    crkit_create_default_engine_config(&config);

    // 设置为自动选择后端（避免TensorRT/ONNX不可用的问题）
    config.backend = CRKIT_BACKEND_AUTO;
    config.device = CRKIT_DEVICE_CPU;  // 使用CPU以确保兼容性

    CRKitEngine engine;
    CRKitStatus status = crkit_create_engine(&engine, &config);

    if (status == CRKIT_SUCCESS) {
        std::cout << "  Engine created successfully" << std::endl;

        // 销毁引擎
        status = crkit_destroy_engine(engine);
        assert(status == CRKIT_SUCCESS);
        std::cout << "  Engine destroyed successfully" << std::endl;
        std::cout << "  ✓ PASS\n" << std::endl;
    } else {
        std::cout << "  ⚠ SKIP (No backend available: " << crkit_get_last_error() << ")" << std::endl;
        std::cout << "  This is expected if TensorRT/ONNX Runtime not installed\n" << std::endl;
    }
}

void test_result_management() {
    std::cout << "Test: Result Management" << std::endl;

    // 测试释放空指针
    CRKitStatus status = crkit_free_result(nullptr);
    assert(status == CRKIT_SUCCESS);

    // 测试批量释放
    status = crkit_free_batch_results(nullptr, 0);
    assert(status == CRKIT_SUCCESS);

    std::cout << "  ✓ PASS\n" << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  CRKit SDK API Test Suite\n";
    std::cout << "========================================\n\n";

    try {
        test_version();
        test_config_creation();
        test_error_handling();
        test_log_control();
        test_engine_lifecycle();
        test_result_management();

        std::cout << "========================================\n";
        std::cout << "  All Tests Passed! ✓\n";
        std::cout << "========================================\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
