/**
 * @file test_backend.cpp
 * @brief 推理后端测试
 */

#include "backend/inference_backend.h"
#include "utils/logger.h"
#include "utils/types.h"
#include <iostream>
#include <cassert>

using namespace crkit;

void test_backend_factory() {
    std::cout << "Test: Backend Factory" << std::endl;

    // 测试后端可用性检查
    bool tensorrt_available = BackendFactory::isAvailable(CRKIT_BACKEND_TENSORRT);
    bool onnx_available = BackendFactory::isAvailable(CRKIT_BACKEND_ONNXRUNTIME);
    bool auto_available = BackendFactory::isAvailable(CRKIT_BACKEND_AUTO);

    std::cout << "  TensorRT available: " << (tensorrt_available ? "YES" : "NO") << std::endl;
    std::cout << "  ONNX Runtime available: " << (onnx_available ? "YES" : "NO") << std::endl;
    std::cout << "  Auto select available: " << (auto_available ? "YES" : "NO") << std::endl;

    assert(auto_available);  // Auto should always be available

    // 如果有可用的后端，尝试创建
    if (tensorrt_available || onnx_available) {
        auto backend = BackendFactory::create(CRKIT_BACKEND_AUTO);
        assert(backend != nullptr);
        std::cout << "  Backend created successfully" << std::endl;
    } else {
        std::cout << "  ⚠ No backends available (expected if dependencies not installed)" << std::endl;
    }

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_tensor_operations() {
    std::cout << "Test: Tensor Operations" << std::endl;

    Tensor tensor;
    tensor.shape = {1, 3, 640, 640};
    tensor.dtype = CRKIT_DTYPE_FLOAT32;
    tensor.device = CRKIT_DEVICE_CPU;

    assert(tensor.batch() == 1);
    assert(tensor.channels() == 3);
    assert(tensor.height() == 640);
    assert(tensor.width() == 640);
    assert(tensor.count() == 1 * 3 * 640 * 640);

    std::cout << "  Tensor shape: [" << tensor.batch() << ", "
              << tensor.channels() << ", " << tensor.height() << ", "
              << tensor.width() << "]" << std::endl;
    std::cout << "  Total elements: " << tensor.count() << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_detection_structure() {
    std::cout << "Test: Detection Structure" << std::endl;

    Detection det;
    det.x = 100.0f;
    det.y = 200.0f;
    det.width = 50.0f;
    det.height = 80.0f;
    det.class_id = 0;
    det.confidence = 0.95f;
    det.class_name = "person";

    assert(det.x == 100.0f);
    assert(det.class_id == 0);
    assert(det.confidence == 0.95f);
    assert(det.class_name == "person");

    std::cout << "  Detection: " << det.class_name << " @ ["
              << det.x << ", " << det.y << ", "
              << det.width << ", " << det.height << "] "
              << "conf=" << det.confidence << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_model_metadata() {
    std::cout << "Test: Model Metadata" << std::endl;

    ModelMetadata metadata;
    metadata.model_path = "test.onnx";
    metadata.input_width = 640;
    metadata.input_height = 640;
    metadata.num_classes = 80;
    metadata.conf_threshold = 0.5f;
    metadata.nms_threshold = 0.45f;
    metadata.keep_aspect_ratio = true;
    metadata.mean[0] = 0.0f;
    metadata.mean[1] = 0.0f;
    metadata.mean[2] = 0.0f;
    metadata.std[0] = 1.0f;
    metadata.std[1] = 1.0f;
    metadata.std[2] = 1.0f;

    assert(metadata.input_width == 640);
    assert(metadata.num_classes == 80);
    assert(metadata.keep_aspect_ratio == true);

    std::cout << "  Model: " << metadata.model_path << std::endl;
    std::cout << "  Input size: " << metadata.input_width << "x" << metadata.input_height << std::endl;
    std::cout << "  Classes: " << metadata.num_classes << std::endl;
    std::cout << "  Thresholds: conf=" << metadata.conf_threshold
              << ", nms=" << metadata.nms_threshold << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

void test_engine_metadata() {
    std::cout << "Test: Engine Metadata" << std::endl;

    EngineMetadata metadata;
    metadata.backend = CRKIT_BACKEND_TENSORRT;
    metadata.device = CRKIT_DEVICE_GPU;
    metadata.device_id = 0;
    metadata.max_batch_size = 4;
    metadata.enable_fp16 = true;
    metadata.num_worker_threads = 4;
    metadata.max_workspace_mb = 2048;
    metadata.log_level = "INFO";

    assert(metadata.backend == CRKIT_BACKEND_TENSORRT);
    assert(metadata.device == CRKIT_DEVICE_GPU);
    assert(metadata.max_batch_size == 4);
    assert(metadata.enable_fp16 == true);

    std::cout << "  Backend: " << static_cast<int>(metadata.backend) << std::endl;
    std::cout << "  Device: GPU:" << metadata.device_id << std::endl;
    std::cout << "  Batch size: " << metadata.max_batch_size << std::endl;
    std::cout << "  FP16: " << (metadata.enable_fp16 ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Threads: " << metadata.num_worker_threads << std::endl;

    std::cout << "  ✓ PASS\n" << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  CRKit SDK Backend Test Suite\n";
    std::cout << "========================================\n\n";

    // 设置日志级别
    Logger::instance().setLevel("INFO");

    try {
        test_backend_factory();
        test_tensor_operations();
        test_detection_structure();
        test_model_metadata();
        test_engine_metadata();

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
