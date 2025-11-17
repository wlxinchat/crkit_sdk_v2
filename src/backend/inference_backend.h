/**
 * @file inference_backend.h
 * @brief 推理后端抽象接口
 */

#ifndef CRKIT_BACKEND_INFERENCE_BACKEND_H
#define CRKIT_BACKEND_INFERENCE_BACKEND_H

#include "utils/types.h"
#include "crkit_api.h"
#include <vector>
#include <memory>

namespace crkit {

/**
 * @brief 推理后端抽象基类
 *
 * 所有推理引擎(TensorRT, ONNX Runtime等)必须实现此接口
 */
class IInferenceBackend {
public:
    virtual ~IInferenceBackend() = default;

    /**
     * @brief 初始化后端
     * @param metadata 引擎元数据
     * @return 状态码
     */
    virtual CRKitStatus initialize(const EngineMetadata& metadata) = 0;

    /**
     * @brief 加载模型
     * @param model_metadata 模型元数据
     * @return 状态码
     */
    virtual CRKitStatus loadModel(const ModelMetadata& model_metadata) = 0;

    /**
     * @brief 执行推理 (单张)
     * @param input 输入张量
     * @param output 输出张量
     * @return 状态码
     */
    virtual CRKitStatus infer(const Tensor& input, Tensor& output) = 0;

    /**
     * @brief 执行批量推理
     * @param inputs 输入张量数组
     * @param outputs 输出张量数组
     * @return 状态码
     */
    virtual CRKitStatus inferBatch(const std::vector<Tensor>& inputs,
                                   std::vector<Tensor>& outputs) = 0;

    /**
     * @brief 获取设备类型
     */
    virtual CRKitDeviceType getDeviceType() const = 0;

    /**
     * @brief 获取后端类型
     */
    virtual CRKitBackendType getBackendType() const = 0;

    /**
     * @brief 获取模型输入形状
     */
    virtual std::vector<int> getInputShape() const = 0;

    /**
     * @brief 获取模型输出形状
     */
    virtual std::vector<int> getOutputShape() const = 0;

    /**
     * @brief 预热模型
     * @param iterations 预热次数
     * @return 状态码
     */
    virtual CRKitStatus warmup(int iterations) = 0;

    /**
     * @brief 获取模型信息(JSON格式)
     */
    virtual std::string getModelInfo() const = 0;
};

/**
 * @brief 后端工厂类
 */
class BackendFactory {
public:
    /**
     * @brief 创建推理后端
     * @param type 后端类型
     * @return 后端实例指针
     */
    static std::unique_ptr<IInferenceBackend> create(CRKitBackendType type);

    /**
     * @brief 检查后端是否可用
     * @param type 后端类型
     * @return true表示可用
     */
    static bool isAvailable(CRKitBackendType type);
};

} // namespace crkit

#endif // CRKIT_BACKEND_INFERENCE_BACKEND_H
