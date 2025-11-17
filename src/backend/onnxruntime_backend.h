/**
 * @file onnxruntime_backend.h
 * @brief ONNX Runtime推理后端
 */

#ifndef CRKIT_BACKEND_ONNXRUNTIME_BACKEND_H
#define CRKIT_BACKEND_ONNXRUNTIME_BACKEND_H

#include "inference_backend.h"

#ifdef CRKIT_ENABLE_ONNXRUNTIME

#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <vector>

namespace crkit {

/**
 * @brief ONNX Runtime推理后端实现
 */
class ONNXRuntimeBackend : public IInferenceBackend {
public:
    ONNXRuntimeBackend();
    ~ONNXRuntimeBackend() override;

    CRKitStatus initialize(const EngineMetadata& metadata) override;
    CRKitStatus loadModel(const ModelMetadata& model_metadata) override;
    CRKitStatus infer(const Tensor& input, Tensor& output) override;
    CRKitStatus inferBatch(const std::vector<Tensor>& inputs,
                           std::vector<Tensor>& outputs) override;
    CRKitDeviceType getDeviceType() const override { return device_type_; }
    CRKitBackendType getBackendType() const override { return CRKIT_BACKEND_ONNXRUNTIME; }
    std::vector<int> getInputShape() const override;
    std::vector<int> getOutputShape() const override;
    CRKitStatus warmup(int iterations) override;
    std::string getModelInfo() const override;

private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> session_options_;
    Ort::MemoryInfo memory_info_{nullptr};

    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
    std::vector<int64_t> input_shape_;
    std::vector<int64_t> output_shape_;

    CRKitDeviceType device_type_ = CRKIT_DEVICE_CPU;
    int device_id_ = 0;
    int num_threads_ = 4;
};

} // namespace crkit

#endif // CRKIT_ENABLE_ONNXRUNTIME

#endif // CRKIT_BACKEND_ONNXRUNTIME_BACKEND_H
