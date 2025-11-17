/**
 * @file test_api_interface.cpp
 * @brief API Interface validation tests
 * @author wlxinchat@gmail.com
 * @date 2025-01-17
 */

#include "../include/crkit_api.h"
#include <cassert>
#include <iostream>
#include <cstring>

// Test result tracking
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

// ===========================
// TC-007: API Basic Function Test
// ===========================
void test_api_config_creation() {
    std::cout << "\n=== TC-007: API Configuration Test ===" << std::endl;

    // Test engine config creation
    CRKitEngineConfig engine_config;
    crkit_create_default_engine_config(&engine_config);

    bool backend_valid = (engine_config.backend == CRKIT_BACKEND_TENSORRT ||
                         engine_config.backend == CRKIT_BACKEND_ONNXRUNTIME ||
                         engine_config.backend == CRKIT_BACKEND_OPENVINO);

    bool device_valid = (engine_config.device == CRKIT_DEVICE_CPU ||
                        engine_config.device == CRKIT_DEVICE_GPU);

    bool device_id_valid = (engine_config.device_id >= 0);
    bool max_batch_valid = (engine_config.max_batch_size > 0);

    record_test("Engine config backend", backend_valid, "Invalid backend type");
    record_test("Engine config device", device_valid, "Invalid device type");
    record_test("Engine config device_id", device_id_valid, "Invalid device ID");
    record_test("Engine config max_batch_size", max_batch_valid, "Invalid batch size");

    // Test model config creation
    CRKitModelConfig model_config;
    crkit_create_default_model_config(&model_config);

    bool model_path_init = (model_config.model_path != nullptr);
    bool input_dims_valid = (model_config.input_width > 0 && model_config.input_height > 0);
    bool num_classes_valid = (model_config.num_classes > 0);
    bool thresholds_valid = (model_config.conf_threshold >= 0.0f &&
                            model_config.conf_threshold <= 1.0f &&
                            model_config.nms_threshold >= 0.0f &&
                            model_config.nms_threshold <= 1.0f);
    bool task_type_valid = (model_config.task_type == CRKIT_TASK_DETECTION ||
                           model_config.task_type == CRKIT_TASK_CLASSIFICATION ||
                           model_config.task_type == CRKIT_TASK_SEGMENTATION);

    record_test("Model config model_path", model_path_init, "Model path not initialized");
    record_test("Model config input dimensions", input_dims_valid, "Invalid input dimensions");
    record_test("Model config num_classes", num_classes_valid, "Invalid number of classes");
    record_test("Model config thresholds", thresholds_valid, "Invalid threshold values");
    record_test("Model config task_type", task_type_valid, "Invalid task type");

    // Test inference config creation
    CRKitInferenceConfig infer_config;
    crkit_create_default_inference_config(&infer_config);

    bool batch_size_valid = (infer_config.batch_size > 0);
    bool mode_valid = (infer_config.mode == CRKIT_INFER_SYNC ||
                      infer_config.mode == CRKIT_INFER_ASYNC);

    record_test("Inference config batch_size", batch_size_valid, "Invalid batch size");
    record_test("Inference config mode", mode_valid, "Invalid inference mode");
}

// ===========================
// TC-016: Error Handling Test
// ===========================
void test_error_handling() {
    std::cout << "\n=== TC-016: Error Handling Test ===" << std::endl;

    // Test NULL pointer handling
    CRKitStatus status;

    // Test NULL engine pointer
    status = crkit_create_engine(nullptr, nullptr);
    record_test("NULL engine pointer handling",
                status == CRKIT_ERROR_INVALID_ARGUMENT,
                "Should return CRKIT_ERROR_INVALID_ARGUMENT");

    // Test NULL config pointer
    CRKitEngine engine;
    status = crkit_create_engine(&engine, nullptr);
    record_test("NULL config pointer handling",
                status == CRKIT_ERROR_INVALID_ARGUMENT,
                "Should return CRKIT_ERROR_INVALID_ARGUMENT");

    // Test error message retrieval
    const char* error_msg = crkit_get_last_error();
    bool has_error_msg = (error_msg != nullptr);
    record_test("Error message retrieval", has_error_msg, "Error message is NULL");

    // Test status to string conversion
    const char* success_str = crkit_status_to_string(CRKIT_SUCCESS);
    const char* error_str = crkit_status_to_string(CRKIT_ERROR_INVALID_ARGUMENT);

    bool status_strings_valid = (success_str != nullptr && error_str != nullptr &&
                                strcmp(success_str, error_str) != 0);
    record_test("Status to string conversion", status_strings_valid,
                "Invalid status string conversion");
}

// ===========================
// TC-API-001: Enum Values Test
// ===========================
void test_enum_values() {
    std::cout << "\n=== TC-API-001: Enum Values Test ===" << std::endl;

    // Test CRKitStatus enum
    bool status_enum_valid = (CRKIT_SUCCESS == 0 &&
                              CRKIT_ERROR_INVALID_ARGUMENT != 0 &&
                              CRKIT_ERROR_OUT_OF_MEMORY != 0 &&
                              CRKIT_ERROR_DEVICE_ERROR != 0);
    record_test("CRKitStatus enum values", status_enum_valid, "Invalid status enum values");

    // Test CRKitBackend enum
    bool backend_enum_valid = (CRKIT_BACKEND_TENSORRT != CRKIT_BACKEND_ONNXRUNTIME &&
                               CRKIT_BACKEND_ONNXRUNTIME != CRKIT_BACKEND_OPENVINO);
    record_test("CRKitBackend enum values", backend_enum_valid, "Backend enum values overlap");

    // Test CRKitDevice enum
    bool device_enum_valid = (CRKIT_DEVICE_CPU != CRKIT_DEVICE_GPU);
    record_test("CRKitDevice enum values", device_enum_valid, "Device enum values overlap");

    // Test CRKitTaskType enum
    bool task_enum_valid = (CRKIT_TASK_DETECTION != CRKIT_TASK_CLASSIFICATION &&
                           CRKIT_TASK_CLASSIFICATION != CRKIT_TASK_SEGMENTATION);
    record_test("CRKitTaskType enum values", task_enum_valid, "Task type enum values overlap");
}

// ===========================
// TC-API-002: Struct Layout Test
// ===========================
void test_struct_layout() {
    std::cout << "\n=== TC-API-002: Struct Layout Test ===" << std::endl;

    // Test CRKitDetection struct
    CRKitDetection detection;
    detection.x = 100.0f;
    detection.y = 200.0f;
    detection.width = 50.0f;
    detection.height = 60.0f;
    detection.confidence = 0.95f;
    detection.class_id = 5;
    detection.class_name = "test";

    bool detection_layout_valid = (detection.x == 100.0f &&
                                   detection.y == 200.0f &&
                                   detection.width == 50.0f &&
                                   detection.height == 60.0f &&
                                   detection.confidence == 0.95f &&
                                   detection.class_id == 5);
    record_test("CRKitDetection struct layout", detection_layout_valid,
                "Struct member access failed");

    // Test CRKitImage struct
    CRKitImage image;
    unsigned char data[100];
    image.data = data;
    image.width = 640;
    image.height = 480;
    image.channels = 3;
    image.format = CRKIT_IMAGE_FORMAT_BGR;

    bool image_layout_valid = (image.data == data &&
                              image.width == 640 &&
                              image.height == 480 &&
                              image.channels == 3);
    record_test("CRKitImage struct layout", image_layout_valid,
                "Image struct member access failed");
}

// ===========================
// TC-API-003: Version Info Test
// ===========================
void test_version_info() {
    std::cout << "\n=== TC-API-003: Version Info Test ===" << std::endl;

    const char* version = crkit_get_version();
    bool has_version = (version != nullptr && strlen(version) > 0);
    record_test("Version string", has_version, "Version string is empty or NULL");

    if (has_version) {
        std::cout << "    SDK Version: " << version << std::endl;
    }

    const char* build_info = crkit_get_build_info();
    bool has_build_info = (build_info != nullptr);
    record_test("Build info", has_build_info, "Build info is NULL");

    if (has_build_info) {
        std::cout << "    Build Info: " << build_info << std::endl;
    }
}

// ===========================
// Main Test Runner
// ===========================
int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        CRKit SDK - API Interface Test Suite              ║" << std::endl;
    std::cout << "║        Version: 1.0.0                                     ║" << std::endl;
    std::cout << "║        Date: 2025-01-17                                   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    // Run all tests
    test_api_config_creation();
    test_error_handling();
    test_enum_values();
    test_struct_layout();
    test_version_info();

    // Print summary
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                      TEST SUMMARY                          ║" << std::endl;
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
        std::cout << "\n✓✓✓ ALL API TESTS PASSED ✓✓✓" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗✗✗ SOME API TESTS FAILED ✗✗✗" << std::endl;
        return 1;
    }
}
