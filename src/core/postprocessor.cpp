/**
 * @file postprocessor.cpp
 * @brief 后处理器实现
 */

#include "postprocessor.h"
#include "utils/logger.h"
#include "utils/error.h"
#include <algorithm>
#include <cmath>

namespace crkit {

Postprocessor::Postprocessor()
    : conf_threshold_(0.5f), nms_threshold_(0.45f),
      num_classes_(80), scale_(1.0f), pad_w_(0), pad_h_(0) {
}

Postprocessor::~Postprocessor() = default;

void Postprocessor::configure(const ModelMetadata& model_metadata) {
    conf_threshold_ = model_metadata.conf_threshold;
    nms_threshold_ = model_metadata.nms_threshold;
    num_classes_ = model_metadata.num_classes;
    class_names_ = model_metadata.class_names;

    LOG_INFO("Postprocessor configured: conf=%.2f, nms=%.2f, classes=%d",
             conf_threshold_, nms_threshold_, num_classes_);
}

void Postprocessor::setPreprocessParams(float scale, int pad_w, int pad_h) {
    scale_ = scale;
    pad_w_ = pad_w;
    pad_h_ = pad_h;
}

CRKitStatus Postprocessor::process(const Tensor& output, std::vector<Detection>& detections) {
    if (!output.data) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid output tensor");
    }

    detections.clear();

    // 1. 解码输出
    std::vector<BBox> boxes;
    decodeOutput(output, boxes);

    // 2. NMS
    nms(boxes, nms_threshold_);

    // 3. 转换为Detection格式并映射坐标
    for (const auto& box : boxes) {
        Detection det;
        det.x = box.x;
        det.y = box.y;
        det.width = box.width;
        det.height = box.height;
        det.class_id = box.class_id;
        det.confidence = box.confidence;

        // 设置类别名称
        if (box.class_id >= 0 && box.class_id < static_cast<int>(class_names_.size())) {
            det.class_name = class_names_[box.class_id];
        } else {
            det.class_name = "class_" + std::to_string(box.class_id);
        }

        // 映射回原图坐标
        mapToOriginal(det);

        detections.push_back(det);
    }

    LOG_DEBUG("Post-processing result: %zu detections", detections.size());

    return CRKIT_SUCCESS;
}

CRKitStatus Postprocessor::processBatch(const std::vector<Tensor>& outputs,
                                        std::vector<std::vector<Detection>>& batch_detections) {
    batch_detections.clear();
    batch_detections.resize(outputs.size());

    for (size_t i = 0; i < outputs.size(); i++) {
        CRKitStatus status = process(outputs[i], batch_detections[i]);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
    }

    return CRKIT_SUCCESS;
}

void Postprocessor::decodeOutput(const Tensor& output, std::vector<BBox>& boxes) {
    boxes.clear();

    // YOLO输出格式: [1, num_boxes, 4+1+num_classes]
    // 或者: [1, num_boxes, 4+num_classes] (objectness已经融合)
    // 简化实现，假设输出格式为 [1, N, 85] (COCO 80类)
    // 每一行: [x, y, w, h, conf, class0_prob, class1_prob, ...]

    const float* data = static_cast<const float*>(output.data);

    if (output.shape.size() < 2) {
        LOG_WARN("Invalid output shape");
        return;
    }

    int num_boxes = output.shape[1];
    int num_attrs = output.shape[2];  // 85 = 4 + 1 + 80

    LOG_DEBUG("Decoding output: num_boxes=%d, num_attrs=%d", num_boxes, num_attrs);

    for (int i = 0; i < num_boxes; i++) {
        const float* row = data + i * num_attrs;

        // 获取最大置信度的类别
        float objectness = row[4];  // 目标置信度

        int best_class = 0;
        float best_score = 0.0f;

        for (int c = 0; c < num_classes_; c++) {
            float score = row[5 + c];
            if (score > best_score) {
                best_score = score;
                best_class = c;
            }
        }

        float confidence = objectness * best_score;

        if (confidence < conf_threshold_) {
            continue;
        }

        // 解析边界框 (center_x, center_y, width, height)
        float cx = row[0];
        float cy = row[1];
        float w = row[2];
        float h = row[3];

        // 转换为 (x, y, width, height) 左上角坐标
        BBox box;
        box.x = cx - w / 2.0f;
        box.y = cy - h / 2.0f;
        box.width = w;
        box.height = h;
        box.class_id = best_class;
        box.confidence = confidence;

        boxes.push_back(box);
    }
}

void Postprocessor::nms(std::vector<BBox>& boxes, float threshold) {
    // 按置信度降序排序
    std::sort(boxes.begin(), boxes.end(),
              [](const BBox& a, const BBox& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<bool> suppressed(boxes.size(), false);

    for (size_t i = 0; i < boxes.size(); i++) {
        if (suppressed[i]) continue;

        for (size_t j = i + 1; j < boxes.size(); j++) {
            if (suppressed[j]) continue;

            // 只对同一类别进行NMS
            if (boxes[i].class_id != boxes[j].class_id) continue;

            float iou_value = iou(boxes[i], boxes[j]);
            if (iou_value > threshold) {
                suppressed[j] = true;
            }
        }
    }

    // 移除被抑制的框
    std::vector<BBox> result;
    for (size_t i = 0; i < boxes.size(); i++) {
        if (!suppressed[i]) {
            result.push_back(boxes[i]);
        }
    }

    boxes = std::move(result);
}

float Postprocessor::iou(const BBox& a, const BBox& b) {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.width, b.x + b.width);
    float y2 = std::min(a.y + a.height, b.y + b.height);

    float intersection = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float area_a = a.width * a.height;
    float area_b = b.width * b.height;
    float union_area = area_a + area_b - intersection;

    return intersection / (union_area + 1e-6f);
}

void Postprocessor::mapToOriginal(Detection& det) {
    if (scale_ <= 0.0f) {
        scale_ = 1.0f;
    }

    // 减去padding
    det.x -= pad_w_;
    det.y -= pad_h_;

    // 缩放回原图
    det.x /= scale_;
    det.y /= scale_;
    det.width /= scale_;
    det.height /= scale_;

    // 确保坐标非负
    det.x = std::max(0.0f, det.x);
    det.y = std::max(0.0f, det.y);
}

} // namespace crkit
