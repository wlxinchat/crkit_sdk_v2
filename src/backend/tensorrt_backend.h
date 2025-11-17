/**
 * @file tensorrt_backend.h
 * @brief TensorRT推理后端
 */

#ifndef CRKIT_BACKEND_TENSORRT_BACKEND_H
#define CRKIT_BACKEND_TENSORRT_BACKEND_H

#include "inference_backend.h"

#ifdef CRKIT_ENABLE_TENSORRT

#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime.h>
#include <memory>
#include <string>

namespace crkit {

/**
 * @brief TensorRT日志器
 */
class TRTLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override;
};

/**
 * @brief TensorRT推理后端实现
 */
class TensorRTBackend : public IInferenceBackend {
public:
    TensorRTBackend();
    ~TensorRTBackend() override;

    CRKitStatus initialize(const EngineMetadata& metadata) override;
    CRKitStatus loadModel(const ModelMetadata& model_metadata) override;
    CRKitStatus infer(const Tensor& input, Tensor& output) override;
    CRKitStatus inferBatch(const std::vector<Tensor>& inputs,
                           std::vector<Tensor>& outputs) override;
    CRKitDeviceType getDeviceType() const override { return device_type_; }
    CRKitBackendType getBackendType() const override { return CRKIT_BACKEND_TENSORRT; }
    std::vector<int> getInputShape() const override;
    std::vector<int> getOutputShape() const override;
    CRKitStatus warmup(int iterations) override;
    std::string getModelInfo() const override;

private:
    CRKitStatus buildEngineFromONNX(const std::string& onnx_path);
    CRKitStatus loadEngineFromFile(const std::string& engine_path);
    CRKitStatus saveEngineToFile(const std::string& engine_path);
    std::string getEngineCachePath(const std::string& onnx_path);

    TRTLogger logger_;
    nvinfer1::IRuntime* runtime_ = nullptr;
    nvinfer1::ICudaEngine* engine_ = nullptr;
    nvinfer1::IExecutionContext* context_ = nullptr;

    cudaStream_t stream_ = nullptr;
    void* device_buffers_[2] = {nullptr, nullptr};  // input, output
    void* host_buffers_[2] = {nullptr, nullptr};

    int input_index_ = 0;
    int output_index_ = 1;
    size_t input_size_ = 0;
    size_t output_size_ = 0;

    CRKitDeviceType device_type_ = CRKIT_DEVICE_GPU;
    int device_id_ = 0;
    int max_batch_size_ = 1;
    bool enable_fp16_ = false;
    int max_workspace_mb_ = 1024;
};

} // namespace crkit

#endif // CRKIT_ENABLE_TENSORRT

#endif // CRKIT_BACKEND_TENSORRT_BACKEND_H
