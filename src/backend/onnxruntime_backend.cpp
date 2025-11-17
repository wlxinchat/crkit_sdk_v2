/**
 * @file onnxruntime_backend.cpp
 * @brief ONNX Runtime推理后端实现
 */

#include "onnxruntime_backend.h"

#ifdef CRKIT_ENABLE_ONNXRUNTIME

#include "utils/logger.h"
#include "utils/error.h"
#include <cstring>
#include <sstream>

namespace crkit {

ONNXRuntimeBackend::ONNXRuntimeBackend()
    : memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
    LOG_INFO("ONNX Runtime backend created");
}

ONNXRuntimeBackend::~ONNXRuntimeBackend() {
    // 智能指针会自动清理
    LOG_INFO("ONNX Runtime backend destroyed");
}

CRKitStatus ONNXRuntimeBackend::initialize(const EngineMetadata& metadata) {
    device_type_ = metadata.device;
    device_id_ = metadata.device_id;
    num_threads_ = metadata.num_worker_threads;

    try {
        // 创建环境
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "CRKit");

        // 创建会话选项
        session_options_ = std::make_unique<Ort::SessionOptions>();
        session_options_->SetIntraOpNumThreads(num_threads_);
        session_options_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // 如果是GPU，添加CUDA Provider
        if (device_type_ == CRKIT_DEVICE_GPU) {
#ifdef USE_CUDA
            OrtCUDAProviderOptions cuda_options;
            cuda_options.device_id = device_id_;
            session_options_->AppendExecutionProvider_CUDA(cuda_options);
            LOG_INFO("ONNX Runtime: CUDA provider enabled on GPU:%d", device_id_);
#else
            LOG_WARN("CUDA provider requested but not available, falling back to CPU");
            device_type_ = CRKIT_DEVICE_CPU;
#endif
        }

        LOG_INFO("ONNX Runtime backend initialized, threads=%d", num_threads_);
        return CRKIT_SUCCESS;

    } catch (const Ort::Exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED,
                            std::string("ONNX Runtime init failed: ") + e.what());
    }
}

CRKitStatus ONNXRuntimeBackend::loadModel(const ModelMetadata& model_metadata) {
    try {
        // 加载模型
#ifdef _WIN32
        std::wstring model_path_w(model_metadata.model_path.begin(),
                                  model_metadata.model_path.end());
        session_ = std::make_unique<Ort::Session>(*env_, model_path_w.c_str(),
                                                   *session_options_);
#else
        session_ = std::make_unique<Ort::Session>(*env_, model_metadata.model_path.c_str(),
                                                   *session_options_);
#endif

        // 获取输入信息
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_input_nodes = session_->GetInputCount();

        if (num_input_nodes > 0) {
            // 获取输入名称
            char* input_name = session_->GetInputName(0, allocator);
            input_names_.push_back(input_name);

            // 获取输入形状
            Ort::TypeInfo type_info = session_->GetInputTypeInfo(0);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            input_shape_ = tensor_info.GetShape();

            LOG_INFO("ONNX Runtime: Input name: %s", input_name);
            LOG_INFO("ONNX Runtime: Input shape: [%lld, %lld, %lld, %lld]",
                     input_shape_[0], input_shape_[1], input_shape_[2], input_shape_[3]);
        }

        // 获取输出信息
        size_t num_output_nodes = session_->GetOutputCount();

        if (num_output_nodes > 0) {
            char* output_name = session_->GetOutputName(0, allocator);
            output_names_.push_back(output_name);

            Ort::TypeInfo type_info = session_->GetOutputTypeInfo(0);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            output_shape_ = tensor_info.GetShape();

            LOG_INFO("ONNX Runtime: Output name: %s", output_name);
        }

        LOG_INFO("ONNX Runtime model loaded: %s", model_metadata.model_path.c_str());
        return CRKIT_SUCCESS;

    } catch (const Ort::Exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_MODEL_LOAD_FAILED,
                            std::string("Failed to load ONNX model: ") + e.what());
    }
}

CRKitStatus ONNXRuntimeBackend::infer(const Tensor& input, Tensor& output) {
    if (!session_) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_NOT_INITIALIZED,
                            "ONNX Runtime session not initialized");
    }

    try {
        // 创建输入张量
        std::vector<int64_t> input_shape_int64;
        for (int dim : input.shape) {
            input_shape_int64.push_back(static_cast<int64_t>(dim));
        }

        size_t input_tensor_size = 1;
        for (int64_t dim : input_shape_int64) {
            input_tensor_size *= dim;
        }

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info_,
            static_cast<float*>(input.data),
            input_tensor_size,
            input_shape_int64.data(),
            input_shape_int64.size()
        );

        // 执行推理
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names_.data(),
            &input_tensor,
            1,
            output_names_.data(),
            1
        );

        // 获取输出
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto output_shape_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        auto output_shape_vec = output_shape_info.GetShape();

        // 计算输出大小
        size_t output_size = 1;
        for (int64_t dim : output_shape_vec) {
            output_size *= dim;
        }

        // 分配输出内存并拷贝
        output.shape.clear();
        for (int64_t dim : output_shape_vec) {
            output.shape.push_back(static_cast<int>(dim));
        }
        output.dtype = CRKIT_DTYPE_FLOAT32;
        output.device = CRKIT_DEVICE_CPU;
        output.size_bytes = output_size * sizeof(float);
        output.data = malloc(output.size_bytes);

        if (!output.data) {
            SET_ERROR_AND_RETURN(CRKIT_ERROR_OUT_OF_MEMORY,
                                "Failed to allocate output tensor");
        }

        std::memcpy(output.data, output_data, output.size_bytes);

        return CRKIT_SUCCESS;

    } catch (const Ort::Exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED,
                            std::string("ONNX Runtime inference failed: ") + e.what());
    }
}

CRKitStatus ONNXRuntimeBackend::inferBatch(const std::vector<Tensor>& inputs,
                                           std::vector<Tensor>& outputs) {
    outputs.resize(inputs.size());

    for (size_t i = 0; i < inputs.size(); i++) {
        CRKitStatus status = infer(inputs[i], outputs[i]);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
    }

    return CRKIT_SUCCESS;
}

std::vector<int> ONNXRuntimeBackend::getInputShape() const {
    std::vector<int> shape;
    for (int64_t dim : input_shape_) {
        shape.push_back(static_cast<int>(dim));
    }
    return shape;
}

std::vector<int> ONNXRuntimeBackend::getOutputShape() const {
    std::vector<int> shape;
    for (int64_t dim : output_shape_) {
        shape.push_back(static_cast<int>(dim));
    }
    return shape;
}

CRKitStatus ONNXRuntimeBackend::warmup(int iterations) {
    LOG_INFO("Warming up ONNX Runtime model with %d iterations", iterations);

    Tensor dummy_input;
    dummy_input.shape = getInputShape();
    dummy_input.dtype = CRKIT_DTYPE_FLOAT32;

    size_t input_size = 1;
    for (int dim : dummy_input.shape) {
        input_size *= dim;
    }
    dummy_input.size_bytes = input_size * sizeof(float);
    dummy_input.data = malloc(dummy_input.size_bytes);
    std::memset(dummy_input.data, 0, dummy_input.size_bytes);

    for (int i = 0; i < iterations; i++) {
        Tensor dummy_output;
        infer(dummy_input, dummy_output);
        if (dummy_output.data) {
            free(dummy_output.data);
        }
    }

    free(dummy_input.data);
    LOG_INFO("ONNX Runtime warmup completed");

    return CRKIT_SUCCESS;
}

std::string ONNXRuntimeBackend::getModelInfo() const {
    std::ostringstream oss;
    oss << "{"
        << "\"backend\": \"ONNX Runtime\", "
        << "\"device\": \"" << (device_type_ == CRKIT_DEVICE_GPU ? "GPU" : "CPU") << "\", "
        << "\"threads\": " << num_threads_
        << "}";
    return oss.str();
}

} // namespace crkit

#endif // CRKIT_ENABLE_ONNXRUNTIME
