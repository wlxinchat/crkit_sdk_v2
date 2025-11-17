/**
 * @file preprocessor.cpp
 * @brief 预处理器实现
 */

#include "preprocessor.h"
#include "utils/logger.h"
#include "utils/error.h"
#include <cstring>

namespace crkit {

Preprocessor::Preprocessor()
    : input_width_(640), input_height_(640),
      keep_aspect_ratio_(true),
      bgr_to_rgb_(true),
      scale_(1.0f), pad_w_(0), pad_h_(0) {
    mean_[0] = mean_[1] = mean_[2] = 0.0f;
    std_[0] = std_[1] = std_[2] = 1.0f;
}

Preprocessor::~Preprocessor() = default;

void Preprocessor::configure(const ModelMetadata& model_metadata) {
    input_width_ = model_metadata.input_width;
    input_height_ = model_metadata.input_height;
    keep_aspect_ratio_ = model_metadata.keep_aspect_ratio;
    std::memcpy(mean_, model_metadata.mean, sizeof(mean_));
    std::memcpy(std_, model_metadata.std, sizeof(std_));

    LOG_INFO("Preprocessor configured: %dx%d, keep_aspect=%d",
             input_width_, input_height_, keep_aspect_ratio_);
}

CRKitStatus Preprocessor::process(const CRKitImage& image, Tensor& output) {
    if (!image.data || image.width <= 0 || image.height <= 0) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid input image");
    }

    // 转换为OpenCV Mat
    cv::Mat mat;
    int cv_type = CV_8UC3;
    int cv_code = -1;

    switch (image.format) {
        case CRKIT_IMAGE_FORMAT_RGB:
            cv_type = CV_8UC3;
            cv_code = cv::COLOR_RGB2BGR;
            break;
        case CRKIT_IMAGE_FORMAT_BGR:
            cv_type = CV_8UC3;
            cv_code = -1;  // 已经是BGR，不需要转换
            break;
        case CRKIT_IMAGE_FORMAT_RGBA:
            cv_type = CV_8UC4;
            cv_code = cv::COLOR_RGBA2BGR;
            break;
        case CRKIT_IMAGE_FORMAT_BGRA:
            cv_type = CV_8UC4;
            cv_code = cv::COLOR_BGRA2BGR;
            break;
        case CRKIT_IMAGE_FORMAT_GRAY:
            cv_type = CV_8UC1;
            cv_code = cv::COLOR_GRAY2BGR;
            break;
        default:
            SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Unsupported image format");
    }

    // 创建Mat (不拷贝数据)
    mat = cv::Mat(image.height, image.width, cv_type, image.data, image.stride);

    // 转换颜色空间
    cv::Mat bgr_mat;
    if (cv_code >= 0) {
        cv::cvtColor(mat, bgr_mat, cv_code);
    } else {
        bgr_mat = mat;
    }

    return process(bgr_mat, output);
}

CRKitStatus Preprocessor::process(const cv::Mat& mat, Tensor& output) {
    if (mat.empty()) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Empty input mat");
    }

    // 1. Resize + Padding
    cv::Mat resized = resizeWithPadding(mat);

    // 2. 归一化
    cv::Mat float_mat;
    resized.convertTo(float_mat, CV_32FC3, 1.0 / 255.0);
    normalize(float_mat);

    // 3. RGB转换 (如果需要)
    if (bgr_to_rgb_) {
        cv::cvtColor(float_mat, float_mat, cv::COLOR_BGR2RGB);
    }

    // 4. 分配输出张量
    output.shape = {1, 3, input_height_, input_width_};
    output.dtype = CRKIT_DTYPE_FLOAT32;
    size_t total_size = 1 * 3 * input_height_ * input_width_ * sizeof(float);
    output.data = malloc(total_size);
    output.size_bytes = total_size;

    if (!output.data) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_OUT_OF_MEMORY, "Failed to allocate output tensor");
    }

    // 5. HWC -> CHW
    hwcToChw(float_mat, static_cast<float*>(output.data));

    return CRKIT_SUCCESS;
}

CRKitStatus Preprocessor::processBatch(const CRKitImage* images, int num_images, Tensor& output) {
    if (!images || num_images <= 0) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid batch input");
    }

    // 分配批量张量
    output.shape = {num_images, 3, input_height_, input_width_};
    output.dtype = CRKIT_DTYPE_FLOAT32;
    size_t total_size = num_images * 3 * input_height_ * input_width_ * sizeof(float);
    output.data = malloc(total_size);
    output.size_bytes = total_size;

    if (!output.data) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_OUT_OF_MEMORY, "Failed to allocate batch tensor");
    }

    float* data_ptr = static_cast<float*>(output.data);
    size_t single_size = 3 * input_height_ * input_width_;

    for (int i = 0; i < num_images; i++) {
        Tensor single_output;
        CRKitStatus status = process(images[i], single_output);
        if (status != CRKIT_SUCCESS) {
            free(output.data);
            output.data = nullptr;
            return status;
        }

        // 拷贝到批量张量
        std::memcpy(data_ptr + i * single_size, single_output.data,
                    single_size * sizeof(float));
        free(single_output.data);
    }

    return CRKIT_SUCCESS;
}

cv::Mat Preprocessor::resizeWithPadding(const cv::Mat& img) {
    int orig_width = img.cols;
    int orig_height = img.rows;

    if (!keep_aspect_ratio_) {
        // 直接拉伸
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(input_width_, input_height_));
        scale_ = 1.0f;
        pad_w_ = pad_h_ = 0;
        return resized;
    }

    // 保持宽高比 (letterbox)
    float scale_w = static_cast<float>(input_width_) / orig_width;
    float scale_h = static_cast<float>(input_height_) / orig_height;
    scale_ = std::min(scale_w, scale_h);

    int new_width = static_cast<int>(orig_width * scale_);
    int new_height = static_cast<int>(orig_height * scale_);

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_width, new_height));

    // 计算padding
    pad_w_ = (input_width_ - new_width) / 2;
    pad_h_ = (input_height_ - new_height) / 2;

    // 创建带padding的图像
    cv::Mat padded(input_height_, input_width_, img.type(), cv::Scalar(114, 114, 114));
    resized.copyTo(padded(cv::Rect(pad_w_, pad_h_, new_width, new_height)));

    return padded;
}

void Preprocessor::normalize(cv::Mat& img) {
    // 应用归一化: (x - mean) / std
    for (int c = 0; c < 3; c++) {
        if (std::abs(mean_[c]) > 1e-6 || std::abs(std_[c] - 1.0f) > 1e-6) {
            // 需要归一化
            for (int h = 0; h < img.rows; h++) {
                for (int w = 0; w < img.cols; w++) {
                    img.at<cv::Vec3f>(h, w)[c] =
                        (img.at<cv::Vec3f>(h, w)[c] - mean_[c]) / std_[c];
                }
            }
        }
    }
}

void Preprocessor::hwcToChw(const cv::Mat& src, float* dst) {
    int H = src.rows;
    int W = src.cols;
    int C = src.channels();

    for (int c = 0; c < C; c++) {
        for (int h = 0; h < H; h++) {
            for (int w = 0; w < W; w++) {
                dst[c * H * W + h * W + w] = src.at<cv::Vec3f>(h, w)[c];
            }
        }
    }
}

} // namespace crkit
