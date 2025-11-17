/**
 * @file backend_factory.cpp
 * @brief 后端工厂实现
 */

#include "inference_backend.h"
#include "utils/logger.h"

#ifdef CRKIT_ENABLE_TENSORRT
#include "tensorrt_backend.h"
#endif

#ifdef CRKIT_ENABLE_ONNXRUNTIME
#include "onnxruntime_backend.h"
#endif

#ifdef CRKIT_ENABLE_OPENVINO
#include "openvino_backend.h"
#endif

namespace crkit {

std::unique_ptr<IInferenceBackend> BackendFactory::create(CRKitBackendType type) {
    LOG_INFO("Creating backend: %d", static_cast<int>(type));

    switch (type) {
#ifdef CRKIT_ENABLE_TENSORRT
        case CRKIT_BACKEND_TENSORRT:
            return std::make_unique<TensorRTBackend>();
#endif

#ifdef CRKIT_ENABLE_ONNXRUNTIME
        case CRKIT_BACKEND_ONNXRUNTIME:
            return std::make_unique<ONNXRuntimeBackend>();
#endif

#ifdef CRKIT_ENABLE_OPENVINO
        case CRKIT_BACKEND_OPENVINO:
            return std::make_unique<OpenVINOBackend>();
#endif

        case CRKIT_BACKEND_AUTO:
            // 自动选择可用的后端
#ifdef CRKIT_ENABLE_TENSORRT
            LOG_INFO("Auto selecting TensorRT backend");
            return std::make_unique<TensorRTBackend>();
#elif defined(CRKIT_ENABLE_ONNXRUNTIME)
            LOG_INFO("Auto selecting ONNX Runtime backend");
            return std::make_unique<ONNXRuntimeBackend>();
#elif defined(CRKIT_ENABLE_OPENVINO)
            LOG_INFO("Auto selecting OpenVINO backend");
            return std::make_unique<OpenVINOBackend>();
#endif
            break;

        default:
            LOG_ERROR("Unsupported backend type: %d", static_cast<int>(type));
            return nullptr;
    }

    return nullptr;
}

bool BackendFactory::isAvailable(CRKitBackendType type) {
    switch (type) {
        case CRKIT_BACKEND_TENSORRT:
#ifdef CRKIT_ENABLE_TENSORRT
            return true;
#else
            return false;
#endif

        case CRKIT_BACKEND_ONNXRUNTIME:
#ifdef CRKIT_ENABLE_ONNXRUNTIME
            return true;
#else
            return false;
#endif

        case CRKIT_BACKEND_OPENVINO:
#ifdef CRKIT_ENABLE_OPENVINO
            return true;
#else
            return false;
#endif

        case CRKIT_BACKEND_AUTO:
            return true;  // 总是尝试自动选择

        default:
            return false;
    }
}

} // namespace crkit
