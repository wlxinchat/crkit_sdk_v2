/**
 * @file types.h
 * @brief 内部通用类型定义
 */

#ifndef CRKIT_UTILS_TYPES_H
#define CRKIT_UTILS_TYPES_H

#include "crkit_api.h"
#include <vector>
#include <string>
#include <memory>

namespace crkit {

// 智能指针类型
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// 张量表示
struct Tensor {
    void* data = nullptr;
    std::vector<int> shape;  // NCHW
    CRKitDataType dtype = CRKIT_DTYPE_FLOAT32;
    CRKitDeviceType device = CRKIT_DEVICE_CPU;
    size_t size_bytes = 0;

    Tensor() = default;

    int batch() const { return shape.size() > 0 ? shape[0] : 0; }
    int channels() const { return shape.size() > 1 ? shape[1] : 0; }
    int height() const { return shape.size() > 2 ? shape[2] : 0; }
    int width() const { return shape.size() > 3 ? shape[3] : 0; }

    size_t count() const {
        size_t c = 1;
        for (int dim : shape) c *= dim;
        return c;
    }
};

// 检测结果内部表示
struct Detection {
    float x, y, width, height;
    int class_id;
    float confidence;
    std::string class_name;

    Detection() : x(0), y(0), width(0), height(0),
                  class_id(-1), confidence(0.0f) {}

    Detection(float x, float y, float w, float h, int cls, float conf)
        : x(x), y(y), width(w), height(h),
          class_id(cls), confidence(conf) {}
};

// 推理结果内部表示
struct InferenceOutput {
    std::vector<Detection> detections;
    float preprocess_time_ms = 0.0f;
    float inference_time_ms = 0.0f;
    float postprocess_time_ms = 0.0f;
    int64_t timestamp = 0;

    InferenceOutput() = default;
};

// 模型元数据
struct ModelMetadata {
    std::string model_path;
    int input_width = 640;
    int input_height = 640;
    int num_classes = 80;
    float conf_threshold = 0.5f;
    float nms_threshold = 0.45f;
    std::vector<std::string> class_names;
    bool keep_aspect_ratio = true;
    float mean[3] = {0.0f, 0.0f, 0.0f};
    float std[3] = {1.0f, 1.0f, 1.0f};
};

// 引擎元数据
struct EngineMetadata {
    CRKitBackendType backend = CRKIT_BACKEND_TENSORRT;
    CRKitDeviceType device = CRKIT_DEVICE_GPU;
    int device_id = 0;
    int max_batch_size = 1;
    bool enable_fp16 = false;
    int num_worker_threads = 4;
    int max_workspace_mb = 1024;
    std::string log_level = "INFO";
};

} // namespace crkit

#endif // CRKIT_UTILS_TYPES_H
