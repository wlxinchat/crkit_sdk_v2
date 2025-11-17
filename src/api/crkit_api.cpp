/**
 * @file crkit_api.cpp
 * @brief C API实现
 */

#include "crkit_api.h"
#include "utils/logger.h"
#include "utils/error.h"
#include "utils/types.h"
#include "utils/timer.h"
#include "backend/inference_backend.h"
#include "core/preprocessor.h"
#include "core/postprocessor.h"
#include <opencv2/opencv.hpp>
#include <cstring>
#include <memory>

using namespace crkit;

// 内部引擎结构
struct CRKitEngineImpl {
    EngineMetadata metadata;
    std::unique_ptr<IInferenceBackend> backend;
};

// 内部模型结构
struct CRKitModelImpl {
    CRKitEngineImpl* engine;
    ModelMetadata metadata;
    std::unique_ptr<Preprocessor> preprocessor;
    std::unique_ptr<Postprocessor> postprocessor;
};

// 版本信息
extern "C" const char* crkit_get_version(void) {
    return "1.0.0";
}

// 错误信息
extern "C" const char* crkit_get_last_error(void) {
    return ErrorHandler::instance().getLastError();
}

// 创建默认引擎配置
extern "C" CRKitStatus crkit_create_default_engine_config(CRKitEngineConfig* config) {
    if (!config) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Config is null");
    }

    config->backend = CRKIT_BACKEND_AUTO;
    config->device = CRKIT_DEVICE_GPU;
    config->device_id = 0;
    config->max_batch_size = 1;
    config->enable_fp16 = true;
    config->num_worker_threads = 4;
    config->max_workspace_mb = 1024;
    config->log_level = "INFO";

    return CRKIT_SUCCESS;
}

// 创建默认模型配置
extern "C" CRKitStatus crkit_create_default_model_config(CRKitModelConfig* config) {
    if (!config) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Config is null");
    }

    config->model_path = nullptr;
    config->input_width = 640;
    config->input_height = 640;
    config->num_classes = 80;
    config->conf_threshold = 0.5f;
    config->nms_threshold = 0.45f;
    config->class_names = nullptr;
    config->keep_aspect_ratio = true;
    config->mean[0] = config->mean[1] = config->mean[2] = 0.0f;
    config->std[0] = config->std[1] = config->std[2] = 1.0f;

    return CRKIT_SUCCESS;
}

// 创建引擎
extern "C" CRKitStatus crkit_create_engine(CRKitEngine* engine,
                                           const CRKitEngineConfig* config) {
    if (!engine || !config) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    try {
        // 设置日志级别
        if (config->log_level) {
            Logger::instance().setLevel(config->log_level);
        }

        auto impl = new CRKitEngineImpl();

        // 填充元数据
        impl->metadata.backend = config->backend;
        impl->metadata.device = config->device;
        impl->metadata.device_id = config->device_id;
        impl->metadata.max_batch_size = config->max_batch_size;
        impl->metadata.enable_fp16 = config->enable_fp16;
        impl->metadata.num_worker_threads = config->num_worker_threads;
        impl->metadata.max_workspace_mb = config->max_workspace_mb;
        impl->metadata.log_level = config->log_level ? config->log_level : "INFO";

        // 创建后端
        impl->backend = BackendFactory::create(config->backend);
        if (!impl->backend) {
            delete impl;
            SET_ERROR_AND_RETURN(CRKIT_ERROR_UNSUPPORTED,
                                "Failed to create backend");
        }

        // 初始化后端
        CRKitStatus status = impl->backend->initialize(impl->metadata);
        if (status != CRKIT_SUCCESS) {
            delete impl;
            return status;
        }

        *engine = impl;
        LOG_INFO("Engine created successfully");
        return CRKIT_SUCCESS;

    } catch (const std::exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED, e.what());
    }
}

// 销毁引擎
extern "C" CRKitStatus crkit_destroy_engine(CRKitEngine engine) {
    if (!engine) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Engine is null");
    }

    auto impl = static_cast<CRKitEngineImpl*>(engine);
    delete impl;

    LOG_INFO("Engine destroyed");
    return CRKIT_SUCCESS;
}

// 加载模型
extern "C" CRKitStatus crkit_load_model(CRKitEngine engine,
                                        CRKitModel* model,
                                        const CRKitModelConfig* config) {
    if (!engine || !model || !config || !config->model_path) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    auto engine_impl = static_cast<CRKitEngineImpl*>(engine);

    try {
        auto model_impl = new CRKitModelImpl();
        model_impl->engine = engine_impl;

        // 填充模型元数据
        model_impl->metadata.model_path = config->model_path;
        model_impl->metadata.input_width = config->input_width;
        model_impl->metadata.input_height = config->input_height;
        model_impl->metadata.num_classes = config->num_classes;
        model_impl->metadata.conf_threshold = config->conf_threshold;
        model_impl->metadata.nms_threshold = config->nms_threshold;
        model_impl->metadata.keep_aspect_ratio = config->keep_aspect_ratio;
        std::memcpy(model_impl->metadata.mean, config->mean, sizeof(config->mean));
        std::memcpy(model_impl->metadata.std, config->std, sizeof(config->std));

        // 类别名称
        if (config->class_names) {
            for (int i = 0; i < config->num_classes; i++) {
                if (config->class_names[i]) {
                    model_impl->metadata.class_names.push_back(config->class_names[i]);
                }
            }
        }

        // 加载模型到后端
        CRKitStatus status = engine_impl->backend->loadModel(model_impl->metadata);
        if (status != CRKIT_SUCCESS) {
            delete model_impl;
            return status;
        }

        // 创建预处理器和后处理器
        model_impl->preprocessor = std::make_unique<Preprocessor>();
        model_impl->preprocessor->configure(model_impl->metadata);

        model_impl->postprocessor = std::make_unique<Postprocessor>();
        model_impl->postprocessor->configure(model_impl->metadata);

        *model = model_impl;
        LOG_INFO("Model loaded successfully: %s", config->model_path);
        return CRKIT_SUCCESS;

    } catch (const std::exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_MODEL_LOAD_FAILED, e.what());
    }
}

// 卸载模型
extern "C" CRKitStatus crkit_unload_model(CRKitModel model) {
    if (!model) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Model is null");
    }

    auto impl = static_cast<CRKitModelImpl*>(model);
    delete impl;

    LOG_INFO("Model unloaded");
    return CRKIT_SUCCESS;
}

// 单图像推理
extern "C" CRKitStatus crkit_infer(CRKitModel model,
                                   const CRKitImage* image,
                                   CRKitResult** result) {
    if (!model || !image || !result) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    auto model_impl = static_cast<CRKitModelImpl*>(model);

    try {
        Timer total_timer;

        // 1. 预处理
        Tensor preprocessed;
        Timer preprocess_timer;
        CRKitStatus status = model_impl->preprocessor->process(*image, preprocessed);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
        float preprocess_time = preprocess_timer.elapsed();

        // 获取预处理参数用于后处理
        float scale = model_impl->preprocessor->getScaleFactor();
        int pad_w, pad_h;
        model_impl->preprocessor->getPadding(pad_w, pad_h);
        model_impl->postprocessor->setPreprocessParams(scale, pad_w, pad_h);

        // 2. 推理
        Tensor inference_output;
        Timer inference_timer;
        status = model_impl->engine->backend->infer(preprocessed, inference_output);
        free(preprocessed.data);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
        float inference_time = inference_timer.elapsed();

        // 3. 后处理
        std::vector<Detection> detections;
        Timer postprocess_timer;
        status = model_impl->postprocessor->process(inference_output, detections);
        free(inference_output.data);
        if (status != CRKIT_SUCCESS) {
            return status;
        }
        float postprocess_time = postprocess_timer.elapsed();

        // 4. 构造结果
        auto result_impl = new CRKitResult();
        result_impl->num_detections = static_cast<int>(detections.size());
        result_impl->preprocess_time_ms = preprocess_time;
        result_impl->inference_time_ms = inference_time;
        result_impl->postprocess_time_ms = postprocess_time;
        result_impl->timestamp = Timer::timestamp();

        if (result_impl->num_detections > 0) {
            result_impl->detections = new CRKitDetection[result_impl->num_detections];
            for (int i = 0; i < result_impl->num_detections; i++) {
                result_impl->detections[i].x = detections[i].x;
                result_impl->detections[i].y = detections[i].y;
                result_impl->detections[i].width = detections[i].width;
                result_impl->detections[i].height = detections[i].height;
                result_impl->detections[i].class_id = detections[i].class_id;
                result_impl->detections[i].confidence = detections[i].confidence;
                result_impl->detections[i].class_name = strdup(detections[i].class_name.c_str());
            }
        } else {
            result_impl->detections = nullptr;
        }

        *result = result_impl;

        LOG_INFO("Inference completed: %d detections, total %.2f ms",
                 result_impl->num_detections, total_timer.elapsed());

        return CRKIT_SUCCESS;

    } catch (const std::exception& e) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INFERENCE_FAILED, e.what());
    }
}

// 从文件推理
extern "C" CRKitStatus crkit_infer_from_file(CRKitModel model,
                                             const char* image_path,
                                             CRKitResult** result) {
    if (!model || !image_path || !result) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    // 使用OpenCV加载图像
    cv::Mat img = cv::imread(image_path);
    if (img.empty()) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_IO_ERROR, "Failed to load image");
    }

    // 转换为CRKitImage
    CRKitImage crkit_img;
    crkit_img.data = img.data;
    crkit_img.width = img.cols;
    crkit_img.height = img.rows;
    crkit_img.channels = img.channels();
    crkit_img.format = CRKIT_IMAGE_FORMAT_BGR;
    crkit_img.stride = static_cast<int>(img.step);

    return crkit_infer(model, &crkit_img, result);
}

// 释放结果
extern "C" CRKitStatus crkit_free_result(CRKitResult* result) {
    if (!result) {
        return CRKIT_SUCCESS;
    }

    if (result->detections) {
        for (int i = 0; i < result->num_detections; i++) {
            if (result->detections[i].class_name) {
                free(const_cast<char*>(result->detections[i].class_name));
            }
        }
        delete[] result->detections;
    }

    delete result;
    return CRKIT_SUCCESS;
}

// 设置日志级别
extern "C" CRKitStatus crkit_set_log_level(const char* level) {
    if (!level) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Level is null");
    }

    Logger::instance().setLevel(level);
    return CRKIT_SUCCESS;
}

// 设置日志文件
extern "C" CRKitStatus crkit_set_log_file(const char* file_path) {
    if (file_path) {
        Logger::instance().setLogFile(file_path);
    } else {
        Logger::instance().closeLogFile();
    }
    return CRKIT_SUCCESS;
}

// 模型预热
extern "C" CRKitStatus crkit_warmup_model(CRKitModel model, int num_iterations) {
    if (!model || num_iterations <= 0) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    auto model_impl = static_cast<CRKitModelImpl*>(model);
    return model_impl->engine->backend->warmup(num_iterations);
}

// 批量推理 (简化实现)
extern "C" CRKitStatus crkit_infer_batch(CRKitModel model,
                                         const CRKitImage* images,
                                         int num_images,
                                         CRKitResult*** results) {
    if (!model || !images || num_images <= 0 || !results) {
        SET_ERROR_AND_RETURN(CRKIT_ERROR_INVALID_PARAM, "Invalid parameters");
    }

    // 简化实现：逐个推理
    *results = new CRKitResult*[num_images];

    for (int i = 0; i < num_images; i++) {
        CRKitStatus status = crkit_infer(model, &images[i], &(*results)[i]);
        if (status != CRKIT_SUCCESS) {
            // 清理已分配的结果
            for (int j = 0; j < i; j++) {
                crkit_free_result((*results)[j]);
            }
            delete[] *results;
            *results = nullptr;
            return status;
        }
    }

    return CRKIT_SUCCESS;
}

// 释放批量结果
extern "C" CRKitStatus crkit_free_batch_results(CRKitResult** results, int num_results) {
    if (!results) {
        return CRKIT_SUCCESS;
    }

    for (int i = 0; i < num_results; i++) {
        crkit_free_result(results[i]);
    }
    delete[] results;

    return CRKIT_SUCCESS;
}
