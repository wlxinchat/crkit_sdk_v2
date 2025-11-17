/**
 * @file tensorrt_backend.cpp
 * @brief TensorRT推理后端实现
 */

#include "tensorrt_backend.h"

#ifdef CRKIT_ENABLE_TENSORRT

#include "utils/logger.h"
#include "utils/error.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace crkit {

// CUDA错误检查宏
#define CUDA_CHECK(call)                                                      \
    do {                                                                      \
        cudaError_t err = call;                                              \
        if (err != cudaSuccess) {                                            \
            LOG_ERROR("CUDA error at %s:%d: %s", __FILE__, __LINE__,        \
                     cudaGetErrorString(err));                               \
            return CRKIT_ERROR_DEVICE_ERROR;                                 \
        }                                                                     \
    } while (0)

// TensorRT Logger实现
void TRTLogger::log(Severity severity, const char* msg) noexcept {
    switch (severity) {
        case Severity::kINTERNAL_ERROR:
        case Severity::kERROR:
            LOG_ERROR("[TRT] %s", msg);
            break;
        case Severity::kWARNING:
            LOG_WARN("[TRT] %s", msg);
            break;
        case Severity::kINFO:
            LOG_INFO("[TRT] %s", msg);
            break;
        case Severity::kVERBOSE:
            LOG_DEBUG("[TRT] %s", msg);
            break;
    }
}

TensorRTBackend::TensorRTBackend() {
    LOG_INFO("TensorRT backend created");
}

TensorRTBackend::~TensorRTBackend() {
    // 清理资源
    if (context_) {
        context_->destroy();
        context_ = nullptr;
    }
    if (engine_) {
        engine_->destroy();
        engine_ = nullptr;
    }
    if (runtime_) {
        runtime_->destroy();
        runtime_ = nullptr;
    }

    if (device_buffers_[0]) cudaFree(device_buffers_[0]);
    if (device_buffers_[1]) cudaFree(device_buffers_[1]);
    if (host_buffers_[0]) cudaFreeHost(host_buffers_[0]);
    if (host_buffers_[1]) cudaFreeHost(host_buffers_[1]);

    if (stream_) {
        cudaStreamDestroy(stream_);
    }

    LOG_INFO("TensorRT backend destroyed");
}

CRKitStatus TensorRTBackend::initialize(const EngineMetadata& metadata) {
    device_type_ = metadata.device;
    device_id_ = metadata.device_id;
    max_batch_size_ = metadata.max_batch_size;
    enable_fp16_ = metadata.enable_fp16;
    max_workspace_mb_ = metadata.max_workspace_mb;

    // 设置CUDA设备
    cudaError_t cuda_status = cudaSetDevice(device_id_);
    if (cuda_status != cudaSuccess) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_DEVICE_ERROR,
                            "Failed to set CUDA device: " +
                            std::string(cudaGetErrorString(cuda_status)));
    }

    // 创建CUDA stream
    cuda_status = cudaStreamCreate(&stream_);
    if (cuda_status != cudaSuccess) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_DEVICE_ERROR,
                            "Failed to create CUDA stream");
    }

    // 创建Runtime
    runtime_ = nvinfer1::createInferRuntime(logger_);
    if (!runtime_) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED,
                            "Failed to create TensorRT runtime");
    }

    LOG_INFO("TensorRT backend initialized on GPU:%d, FP16:%d",
             device_id_, enable_fp16_);

    return CRKIT_SUCCESS;
}

CRKitStatus TensorRTBackend::loadModel(const ModelMetadata& model_metadata) {
    std::string model_path = model_metadata.model_path;

    // 检查是否有缓存的engine文件
    std::string engine_path = getEngineCachePath(model_path);

    std::ifstream engine_file(engine_path, std::ios::binary);
    if (engine_file.good()) {
        LOG_INFO("Loading cached TensorRT engine: %s", engine_path.c_str());
        CRKitStatus status = loadEngineFromFile(engine_path);
        if (status == CRKIT_SUCCESS) {
            goto create_context;
        }
        LOG_WARN("Failed to load cached engine, rebuilding...");
    }

    // 从ONNX构建engine
    LOG_INFO("Building TensorRT engine from ONNX: %s", model_path.c_str());
    if (buildEngineFromONNX(model_path) != CRKIT_SUCCESS) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_MODEL_LOAD_FAILED,
                            "Failed to build TensorRT engine");
    }

    // 保存engine到缓存
    saveEngineToFile(engine_path);

create_context:
    // 创建执行上下文
    context_ = engine_->createExecutionContext();
    if (!context_) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_MODEL_LOAD_FAILED,
                            "Failed to create execution context");
    }

    // 获取输入输出信息
    input_index_ = engine_->getBindingIndex("images");  // YOLO输入名称
    output_index_ = engine_->getBindingIndex("output0");  // YOLO输出名称

    if (input_index_ < 0 || output_index_ < 0) {
        // 尝试使用默认索引
        input_index_ = 0;
        output_index_ = 1;
    }

    nvinfer1::Dims input_dims = engine_->getBindingDimensions(input_index_);
    nvinfer1::Dims output_dims = engine_->getBindingDimensions(output_index_);

    // 计算缓冲区大小
    input_size_ = 1;
    for (int i = 0; i < input_dims.nbDims; i++) {
        input_size_ *= input_dims.d[i];
    }
    input_size_ *= sizeof(float);

    output_size_ = 1;
    for (int i = 0; i < output_dims.nbDims; i++) {
        output_size_ *= output_dims.d[i];
    }
    output_size_ *= sizeof(float);

    // 分配GPU内存
    CUDA_CHECK(cudaMalloc(&device_buffers_[0], input_size_));
    CUDA_CHECK(cudaMalloc(&device_buffers_[1], output_size_));

    // 分配主机内存 (固定内存以提高传输速度)
    CUDA_CHECK(cudaMallocHost(&host_buffers_[0], input_size_));
    CUDA_CHECK(cudaMallocHost(&host_buffers_[1], output_size_));

    LOG_INFO("TensorRT model loaded successfully");
    LOG_INFO("Input size: %zu bytes, Output size: %zu bytes",
             input_size_, output_size_);

    return CRKIT_SUCCESS;
}

CRKitStatus TensorRTBackend::infer(const Tensor& input, Tensor& output) {
    if (!context_ || !engine_) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_NOT_INITIALIZED,
                            "TensorRT backend not initialized");
    }

    // 拷贝输入数据到GPU
    CUDA_CHECK(cudaMemcpyAsync(device_buffers_[0], input.data, input_size_,
                               cudaMemcpyHostToDevice, stream_));

    // 执行推理
    void* bindings[2] = {device_buffers_[0], device_buffers_[1]};
    bool status = context_->enqueueV2(bindings, stream_, nullptr);

    if (!status) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED,
                            "TensorRT inference failed");
    }

    // 拷贝输出数据到主机
    CUDA_CHECK(cudaMemcpyAsync(host_buffers_[1], device_buffers_[1], output_size_,
                               cudaMemcpyDeviceToHost, stream_));

    // 等待完成
    CUDA_CHECK(cudaStreamSynchronize(stream_));

    // 设置输出张量
    output.shape = getOutputShape();
    output.dtype = CRKIT_DTYPE_FLOAT32;
    output.device = CRKIT_DEVICE_CPU;
    output.size_bytes = output_size_;
    output.data = malloc(output_size_);
    std::memcpy(output.data, host_buffers_[1], output_size_);

    return CRKIT_SUCCESS;
}

CRKitStatus TensorRTBackend::inferBatch(const std::vector<Tensor>& inputs,
                                        std::vector<Tensor>& outputs) {
    // 简化实现：逐个推理
    outputs.resize(inputs.size());
    for (size_t i = 0; i < inputs.size(); i++) {
        CRKitStatus status = infer(inputs[i], outputs[i]);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
    }
    return CRKIT_SUCCESS;
}

std::vector<int> TensorRTBackend::getInputShape() const {
    if (!engine_) return {};
    nvinfer1::Dims dims = engine_->getBindingDimensions(input_index_);
    std::vector<int> shape;
    for (int i = 0; i < dims.nbDims; i++) {
        shape.push_back(dims.d[i]);
    }
    return shape;
}

std::vector<int> TensorRTBackend::getOutputShape() const {
    if (!engine_) return {};
    nvinfer1::Dims dims = engine_->getBindingDimensions(output_index_);
    std::vector<int> shape;
    for (int i = 0; i < dims.nbDims; i++) {
        shape.push_back(dims.d[i]);
    }
    return shape;
}

CRKitStatus TensorRTBackend::warmup(int iterations) {
    LOG_INFO("Warming up model with %d iterations", iterations);

    Tensor dummy_input;
    dummy_input.shape = getInputShape();
    dummy_input.dtype = CRKIT_DTYPE_FLOAT32;
    dummy_input.size_bytes = input_size_;
    dummy_input.data = malloc(input_size_);
    std::memset(dummy_input.data, 0, input_size_);

    for (int i = 0; i < iterations; i++) {
        Tensor dummy_output;
        infer(dummy_input, dummy_output);
        if (dummy_output.data) {
            free(dummy_output.data);
        }
    }

    free(dummy_input.data);

    LOG_INFO("Warmup completed");
    return CRKIT_SUCCESS;
}

std::string TensorRTBackend::getModelInfo() const {
    std::ostringstream oss;
    oss << "{"
        << "\"backend\": \"TensorRT\", "
        << "\"device\": \"GPU:" << device_id_ << "\", "
        << "\"fp16\": " << (enable_fp16_ ? "true" : "false") << ", "
        << "\"max_batch\": " << max_batch_size_
        << "}";
    return oss.str();
}

CRKitStatus TensorRTBackend::buildEngineFromONNX(const std::string& onnx_path) {
    // 创建Builder
    nvinfer1::IBuilder* builder = nvinfer1::createInferBuilder(logger_);
    if (!builder) {
        return CRKIT_ERROR_MODEL_LOAD_FAILED;
    }

    // 创建Network
    const auto explicitBatch = 1U << static_cast<uint32_t>(
        nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    nvinfer1::INetworkDefinition* network = builder->createNetworkV2(explicitBatch);

    // ONNX Parser
    nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, logger_);
    if (!parser->parseFromFile(onnx_path.c_str(),
                                static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        LOG_ERROR("Failed to parse ONNX file");
        parser->destroy();
        network->destroy();
        builder->destroy();
        return CRKIT_ERROR_INVALID_MODEL;
    }

    // Builder配置
    nvinfer1::IBuilderConfig* config = builder->createBuilderConfig();
    config->setMaxWorkspaceSize(max_workspace_mb_ * 1024ULL * 1024ULL);

    if (enable_fp16_ && builder->platformHasFastFp16()) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
        LOG_INFO("FP16 mode enabled");
    }

    // 构建引擎
    LOG_INFO("Building TensorRT engine (this may take a while)...");
    engine_ = builder->buildEngineWithConfig(*network, *config);

    config->destroy();
    parser->destroy();
    network->destroy();
    builder->destroy();

    if (!engine_) {
        return CRKIT_ERROR_MODEL_LOAD_FAILED;
    }

    LOG_INFO("TensorRT engine built successfully");
    return CRKIT_SUCCESS;
}

CRKitStatus TensorRTBackend::loadEngineFromFile(const std::string& engine_path) {
    std::ifstream file(engine_path, std::ios::binary);
    if (!file.good()) {
        return CRKIT_ERROR_IO_ERROR;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    engine_ = runtime_->deserializeCudaEngine(buffer.data(), size);
    if (!engine_) {
        return CRKIT_ERROR_MODEL_LOAD_FAILED;
    }

    return CRKIT_SUCCESS;
}

CRKitStatus TensorRTBackend::saveEngineToFile(const std::string& engine_path) {
    if (!engine_) {
        return CRKIT_ERROR_NOT_INITIALIZED;
    }

    nvinfer1::IHostMemory* serialized = engine_->serialize();
    if (!serialized) {
        return CRKIT_ERROR_INFERENCE_FAILED;
    }

    std::ofstream file(engine_path, std::ios::binary);
    if (!file.good()) {
        serialized->destroy();
        return CRKIT_ERROR_IO_ERROR;
    }

    file.write(static_cast<const char*>(serialized->data()), serialized->size());
    file.close();
    serialized->destroy();

    LOG_INFO("TensorRT engine saved to: %s", engine_path.c_str());
    return CRKIT_SUCCESS;
}

std::string TensorRTBackend::getEngineCachePath(const std::string& onnx_path) {
    // 生成缓存文件路径: model.onnx -> model.onnx.trt
    return onnx_path + ".trt";
}

} // namespace crkit

#endif // CRKIT_ENABLE_TENSORRT
