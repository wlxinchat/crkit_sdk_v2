/**
 * @file preprocessor.h
 * @brief 图像预处理Pipeline
 */

#ifndef CRKIT_CORE_PREPROCESSOR_H
#define CRKIT_CORE_PREPROCESSOR_H

#include "crkit_api.h"
#include "utils/types.h"
#include <opencv2/opencv.hpp>
#include <vector>

namespace crkit {

/**
 * @brief 预处理器
 *
 * 负责将输入图像转换为模型需要的格式:
 * 1. Resize (保持宽高比或拉伸)
 * 2. Padding (letterbox)
 * 3. 归一化
 * 4. HWC -> CHW
 * 5. BGR -> RGB (可选)
 */
class Preprocessor {
public:
    Preprocessor();
    ~Preprocessor();

    /**
     * @brief 配置预处理器
     * @param model_metadata 模型元数据
     */
    void configure(const ModelMetadata& model_metadata);

    /**
     * @brief 预处理单张图像
     * @param image 输入图像
     * @param output 输出张量
     * @return 状态码
     */
    CRKitStatus process(const CRKitImage& image, Tensor& output);

    /**
     * @brief 预处理单张图像 (OpenCV Mat)
     * @param mat 输入Mat
     * @param output 输出张量
     * @return 状态码
     */
    CRKitStatus process(const cv::Mat& mat, Tensor& output);

    /**
     * @brief 批量预处理
     * @param images 输入图像数组
     * @param num_images 图像数量
     * @param output 输出批量张量
     * @return 状态码
     */
    CRKitStatus processBatch(const CRKitImage* images, int num_images, Tensor& output);

    /**
     * @brief 获取缩放因子 (用于后处理坐标映射)
     */
    float getScaleFactor() const { return scale_; }

    /**
     * @brief 获取padding偏移
     */
    void getPadding(int& pad_w, int& pad_h) const {
        pad_w = pad_w_;
        pad_h = pad_h_;
    }

private:
    cv::Mat resizeWithPadding(const cv::Mat& img);
    void normalize(cv::Mat& img);
    void hwcToChw(const cv::Mat& src, float* dst);

    int input_width_;
    int input_height_;
    bool keep_aspect_ratio_;
    float mean_[3];
    float std_[3];
    bool bgr_to_rgb_;

    // 用于坐标映射的参数
    float scale_;
    int pad_w_;
    int pad_h_;
};

} // namespace crkit

#endif // CRKIT_CORE_PREPROCESSOR_H
