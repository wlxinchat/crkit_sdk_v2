/**
 * @file simple_inference.c
 * @brief 简单推理示例
 *
 * 演示如何使用CRKit SDK进行单张图像推理
 */

#include "crkit_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    CRKitStatus status;

    // 检查参数
    if (argc < 3) {
        printf("Usage: %s <model_path> <image_path>\n", argv[0]);
        printf("Example: %s yolo11n.onnx test.jpg\n", argv[0]);
        return -1;
    }

    const char* model_path = argv[1];
    const char* image_path = argv[2];

    printf("=== CRKit Simple Inference Example ===\n");
    printf("SDK Version: %s\n", crkit_get_version());
    printf("Model: %s\n", model_path);
    printf("Image: %s\n", image_path);
    printf("\n");

    // 1. 设置日志级别
    crkit_set_log_level("INFO");

    // 2. 创建引擎配置
    CRKitEngineConfig engine_config;
    status = crkit_create_default_engine_config(&engine_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to create engine config: %s\n", crkit_get_last_error());
        return -1;
    }

    // 配置引擎参数
    engine_config.backend = CRKIT_BACKEND_TENSORRT;  // 使用TensorRT
    engine_config.device = CRKIT_DEVICE_GPU;         // 使用GPU
    engine_config.device_id = 0;                     // GPU 0
    engine_config.max_batch_size = 1;
    engine_config.enable_fp16 = 1;                   // 启用FP16加速
    engine_config.max_workspace_mb = 2048;           // 2GB工作空间

    printf("Engine Configuration:\n");
    printf("  Backend: TensorRT\n");
    printf("  Device: GPU:%d\n", engine_config.device_id);
    printf("  FP16: %s\n", engine_config.enable_fp16 ? "Enabled" : "Disabled");
    printf("\n");

    // 3. 创建推理引擎
    CRKitEngine engine;
    status = crkit_create_engine(&engine, &engine_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to create engine: %s\n", crkit_get_last_error());
        return -1;
    }
    printf("✓ Engine created successfully\n");

    // 4. 创建模型配置
    CRKitModelConfig model_config;
    status = crkit_create_default_model_config(&model_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to create model config: %s\n", crkit_get_last_error());
        crkit_destroy_engine(engine);
        return -1;
    }

    // 配置模型参数 (YOLO11)
    model_config.model_path = model_path;
    model_config.input_width = 640;
    model_config.input_height = 640;
    model_config.num_classes = 80;              // COCO数据集80类
    model_config.conf_threshold = 0.5f;         // 置信度阈值
    model_config.nms_threshold = 0.45f;         // NMS阈值
    model_config.keep_aspect_ratio = 1;         // 保持宽高比

    // COCO类别名称 (部分)
    const char* class_names[] = {
        "person", "bicycle", "car", "motorcycle", "airplane",
        "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird",
        "cat", "dog", "horse", "sheep", "cow",
        // ... 省略其他类别
    };
    model_config.class_names = class_names;

    printf("Model Configuration:\n");
    printf("  Input Size: %dx%d\n", model_config.input_width, model_config.input_height);
    printf("  Classes: %d\n", model_config.num_classes);
    printf("  Conf Threshold: %.2f\n", model_config.conf_threshold);
    printf("  NMS Threshold: %.2f\n", model_config.nms_threshold);
    printf("\n");

    // 5. 加载模型
    printf("Loading model... (this may take a while for first time)\n");
    CRKitModel model;
    status = crkit_load_model(engine, &model, &model_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to load model: %s\n", crkit_get_last_error());
        crkit_destroy_engine(engine);
        return -1;
    }
    printf("✓ Model loaded successfully\n");

    // 6. 模型预热
    printf("Warming up model...\n");
    status = crkit_warmup_model(model, 5);
    if (status != CRKIT_SUCCESS) {
        printf("Warning: Model warmup failed: %s\n", crkit_get_last_error());
    }
    printf("✓ Model warmed up\n\n");

    // 7. 执行推理
    printf("Running inference on: %s\n", image_path);
    CRKitResult* result;
    status = crkit_infer_from_file(model, image_path, &result);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to run inference: %s\n", crkit_get_last_error());
        crkit_unload_model(model);
        crkit_destroy_engine(engine);
        return -1;
    }

    // 8. 处理结果
    printf("\n=== Inference Results ===\n");
    printf("Detections: %d\n", result->num_detections);
    printf("Performance:\n");
    printf("  Preprocess:  %.2f ms\n", result->preprocess_time_ms);
    printf("  Inference:   %.2f ms\n", result->inference_time_ms);
    printf("  Postprocess: %.2f ms\n", result->postprocess_time_ms);
    printf("  Total:       %.2f ms\n",
           result->preprocess_time_ms + result->inference_time_ms + result->postprocess_time_ms);
    printf("  FPS:         %.1f\n",
           1000.0f / (result->preprocess_time_ms + result->inference_time_ms + result->postprocess_time_ms));
    printf("\n");

    if (result->num_detections > 0) {
        printf("Detected Objects:\n");
        printf("%-4s %-15s %-10s %-40s\n", "No.", "Class", "Conf", "BBox [x, y, w, h]");
        printf("--------------------------------------------------------------------\n");

        for (int i = 0; i < result->num_detections; i++) {
            CRKitDetection* det = &result->detections[i];
            printf("%-4d %-15s %-10.2f [%.1f, %.1f, %.1f, %.1f]\n",
                   i + 1,
                   det->class_name,
                   det->confidence,
                   det->x, det->y, det->width, det->height);
        }
    } else {
        printf("No objects detected.\n");
    }

    // 9. 清理资源
    printf("\nCleaning up...\n");
    crkit_free_result(result);
    crkit_unload_model(model);
    crkit_destroy_engine(engine);

    printf("✓ Done!\n");
    return 0;
}
