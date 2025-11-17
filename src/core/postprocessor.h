/**
 * @file postprocessor.h
 * @brief 后处理Pipeline (目标检测)
 */

#ifndef CRKIT_CORE_POSTPROCESSOR_H
#define CRKIT_CORE_POSTPROCESSOR_H

#include "crkit_api.h"
#include "utils/types.h"
#include <vector>

namespace crkit {

/**
 * @brief 后处理器 (目标检测)
 *
 * 负责处理模型输出:
 * 1. 解码Bounding Box
 * 2. 置信度过滤
 * 3. NMS (非极大值抑制)
 * 4. 坐标映射回原图
 */
class Postprocessor {
public:
    Postprocessor();
    ~Postprocessor();

    /**
     * @brief 配置后处理器
     * @param model_metadata 模型元数据
     */
    void configure(const ModelMetadata& model_metadata);

    /**
     * @brief 设置预处理参数 (用于坐标映射)
     * @param scale 缩放因子
     * @param pad_w padding宽度
     * @param pad_h padding高度
     */
    void setPreprocessParams(float scale, int pad_w, int pad_h);

    /**
     * @brief 处理模型输出
     * @param output 模型输出张量
     * @param detections 检测结果
     * @return 状态码
     */
    CRKitStatus process(const Tensor& output, std::vector<Detection>& detections);

    /**
     * @brief 批量处理
     * @param outputs 模型输出张量数组
     * @param batch_detections 批量检测结果
     * @return 状态码
     */
    CRKitStatus processBatch(const std::vector<Tensor>& outputs,
                             std::vector<std::vector<Detection>>& batch_detections);

private:
    struct BBox {
        float x, y, width, height;
        int class_id;
        float confidence;
    };

    void decodeOutput(const Tensor& output, std::vector<BBox>& boxes);
    void nms(std::vector<BBox>& boxes, float threshold);
    float iou(const BBox& a, const BBox& b);
    void mapToOriginal(Detection& det);

    float conf_threshold_;
    float nms_threshold_;
    int num_classes_;
    std::vector<std::string> class_names_;

    // 坐标映射参数
    float scale_;
    int pad_w_;
    int pad_h_;
};

} // namespace crkit

#endif // CRKIT_CORE_POSTPROCESSOR_H
