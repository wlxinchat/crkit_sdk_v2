/**
 * @file batch_inference.c
 * @brief 批量推理示例
 *
 * 演示如何使用CRKit SDK进行批量图像推理
 */

#include "crkit_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    CRKitStatus status;

    if (argc < 3) {
        printf("Usage: %s <model_path> <image1> [image2] [image3] ...\n", argv[0]);
        printf("Example: %s yolo11n.onnx img1.jpg img2.jpg img3.jpg\n", argv[0]);
        return -1;
    }

    const char* model_path = argv[1];
    int num_images = argc - 2;
    char** image_paths = &argv[2];

    printf("=== CRKit Batch Inference Example ===\n");
    printf("SDK Version: %s\n", crkit_get_version());
    printf("Model: %s\n", model_path);
    printf("Number of images: %d\n", num_images);
    printf("\n");

    // 创建引擎
    CRKitEngineConfig engine_config;
    crkit_create_default_engine_config(&engine_config);
    engine_config.backend = CRKIT_BACKEND_TENSORRT;
    engine_config.device = CRKIT_DEVICE_GPU;
    engine_config.device_id = 0;
    engine_config.enable_fp16 = 1;
    engine_config.max_batch_size = 4;  // 支持批量推理

    CRKitEngine engine;
    status = crkit_create_engine(&engine, &engine_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to create engine: %s\n", crkit_get_last_error());
        return -1;
    }

    // 加载模型
    CRKitModelConfig model_config;
    crkit_create_default_model_config(&model_config);
    model_config.model_path = model_path;
    model_config.input_width = 640;
    model_config.input_height = 640;
    model_config.num_classes = 80;
    model_config.conf_threshold = 0.5f;
    model_config.nms_threshold = 0.45f;

    CRKitModel model;
    status = crkit_load_model(engine, &model, &model_config);
    if (status != CRKIT_SUCCESS) {
        printf("Failed to load model: %s\n", crkit_get_last_error());
        crkit_destroy_engine(engine);
        return -1;
    }

    printf("✓ Model loaded successfully\n");
    printf("Warming up...\n");
    crkit_warmup_model(model, 3);

    // 批量推理所有图像
    printf("\nRunning batch inference on %d images...\n\n", num_images);

    float total_time = 0.0f;
    int total_detections = 0;

    for (int i = 0; i < num_images; i++) {
        printf("[%d/%d] Processing: %s\n", i + 1, num_images, image_paths[i]);

        CRKitResult* result;
        status = crkit_infer_from_file(model, image_paths[i], &result);

        if (status != CRKIT_SUCCESS) {
            printf("  ✗ Failed: %s\n\n", crkit_get_last_error());
            continue;
        }

        float inference_time = result->inference_time_ms +
                              result->preprocess_time_ms +
                              result->postprocess_time_ms;

        printf("  ✓ Detections: %d\n", result->num_detections);
        printf("  ✓ Time: %.2f ms (%.1f FPS)\n", inference_time, 1000.0f / inference_time);

        // 显示检测结果
        if (result->num_detections > 0) {
            printf("  Objects detected:\n");
            for (int j = 0; j < result->num_detections && j < 5; j++) {  // 最多显示5个
                CRKitDetection* det = &result->detections[j];
                printf("    - %s (%.2f)\n", det->class_name, det->confidence);
            }
            if (result->num_detections > 5) {
                printf("    ... and %d more\n", result->num_detections - 5);
            }
        }
        printf("\n");

        total_time += inference_time;
        total_detections += result->num_detections;

        crkit_free_result(result);
    }

    // 统计信息
    printf("=== Summary ===\n");
    printf("Total images processed: %d\n", num_images);
    printf("Total detections: %d\n", total_detections);
    printf("Average time per image: %.2f ms\n", total_time / num_images);
    printf("Average FPS: %.1f\n", 1000.0f * num_images / total_time);

    // 清理
    crkit_unload_model(model);
    crkit_destroy_engine(engine);

    return 0;
}
