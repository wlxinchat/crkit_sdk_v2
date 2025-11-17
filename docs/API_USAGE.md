# CRKit SDK API 使用指南

## 快速开始

### 1. 基本使用流程

```c
#include "crkit_api.h"

// 1. 创建引擎配置
CRKitEngineConfig engine_config;
crkit_create_default_engine_config(&engine_config);
engine_config.backend = CRKIT_BACKEND_TENSORRT;
engine_config.device = CRKIT_DEVICE_GPU;
engine_config.device_id = 0;
engine_config.max_batch_size = 4;
engine_config.enable_fp16 = true;

// 2. 创建推理引擎
CRKitEngine engine;
CRKitStatus status = crkit_create_engine(&engine, &engine_config);
if (status != CRKIT_SUCCESS) {
    printf("Error: %s\n", crkit_get_last_error());
    return -1;
}

// 3. 配置模型
CRKitModelConfig model_config;
crkit_create_default_model_config(&model_config);
model_config.model_path = "/path/to/model.onnx";
model_config.input_width = 640;
model_config.input_height = 640;
model_config.num_classes = 80;
model_config.conf_threshold = 0.5;
model_config.nms_threshold = 0.45;

// 4. 加载模型
CRKitModel model;
status = crkit_load_model(engine, &model, &model_config);

// 5. 执行推理
CRKitResult* result;
status = crkit_infer_from_file(model, "test.jpg", &result);

// 6. 处理结果
for (int i = 0; i < result->num_detections; i++) {
    CRKitDetection* det = &result->detections[i];
    printf("Class: %s, Conf: %.2f, Box: [%.1f, %.1f, %.1f, %.1f]\n",
           det->class_name, det->confidence,
           det->x, det->y, det->width, det->height);
}

// 7. 清理资源
crkit_free_result(result);
crkit_unload_model(model);
crkit_destroy_engine(engine);
```

## 使用场景示例

### 场景1: 单图像推理

```c
#include "crkit_api.h"
#include <stdio.h>

int main() {
    // 初始化引擎和模型 (省略...)

    // 准备图像数据
    CRKitImage image;
    image.data = /* 图像数据指针 */;
    image.width = 1920;
    image.height = 1080;
    image.channels = 3;
    image.format = CRKIT_IMAGE_FORMAT_BGR;
    image.stride = image.width * image.channels;

    // 执行推理
    CRKitResult* result;
    CRKitStatus status = crkit_infer(model, &image, &result);

    if (status == CRKIT_SUCCESS) {
        printf("检测到 %d 个目标\n", result->num_detections);
        printf("推理耗时: %.2f ms\n", result->inference_time_ms);

        // 处理检测结果
        for (int i = 0; i < result->num_detections; i++) {
            CRKitDetection* det = &result->detections[i];
            // 处理每个检测框...
        }

        crkit_free_result(result);
    }

    return 0;
}
```

### 场景2: 批量推理

```c
#include "crkit_api.h"

void batch_inference_example(CRKitModel model) {
    const int BATCH_SIZE = 4;
    CRKitImage images[BATCH_SIZE];

    // 准备批量图像数据
    for (int i = 0; i < BATCH_SIZE; i++) {
        // 加载或准备图像数据
        images[i].data = /* ... */;
        images[i].width = 640;
        images[i].height = 640;
        images[i].channels = 3;
        images[i].format = CRKIT_IMAGE_FORMAT_RGB;
        images[i].stride = images[i].width * images[i].channels;
    }

    // 执行批量推理
    CRKitResult** results;
    CRKitStatus status = crkit_infer_batch(model, images, BATCH_SIZE, &results);

    if (status == CRKIT_SUCCESS) {
        // 处理每张图像的结果
        for (int i = 0; i < BATCH_SIZE; i++) {
            printf("图像 %d: 检测到 %d 个目标\n", i, results[i]->num_detections);
            // 处理检测结果...
        }

        // 释放结果
        crkit_free_batch_results(results, BATCH_SIZE);
    }
}
```

### 场景3: 异步推理

```c
#include "crkit_api.h"
#include <stdio.h>

// 异步回调函数
void on_inference_complete(const CRKitResult* result, void* user_data) {
    int* frame_id = (int*)user_data;
    printf("帧 %d 推理完成, 检测到 %d 个目标\n",
           *frame_id, result->num_detections);

    // 处理结果...
}

void async_inference_example(CRKitModel model) {
    CRKitImage image;
    // 准备图像数据...

    int frame_id = 123;
    CRKitAsyncTask task;

    // 提交异步任务
    CRKitStatus status = crkit_infer_async(
        model, &image, on_inference_complete, &frame_id, &task);

    if (status == CRKIT_SUCCESS) {
        printf("异步任务已提交\n");

        // 方式1: 等待任务完成
        CRKitResult* result;
        status = crkit_wait_async_task(task, 5000, &result);  // 5秒超时
        if (status == CRKIT_SUCCESS) {
            // 处理结果...
            crkit_free_result(result);
        }

        // 方式2: 不等待，让回调函数处理 (已在上面的回调中处理)
    }
}
```

### 场景4: 视频流推理

```c
#include "crkit_api.h"

typedef struct {
    int frame_count;
    FILE* log_file;
} StreamContext;

void on_frame_processed(const CRKitResult* result, void* user_data) {
    StreamContext* ctx = (StreamContext*)user_data;
    ctx->frame_count++;

    fprintf(ctx->log_file, "Frame %d: %d detections, %.2f ms\n",
            ctx->frame_count, result->num_detections,
            result->inference_time_ms);

    // 处理检测结果，例如显示、保存、发送到上位机等
}

void stream_inference_example(CRKitModel model) {
    CRKitInputStream stream;

    // 打开视频流 (可以是文件、RTSP URL、USB摄像头)
    const char* source = "rtsp://192.168.1.100:554/stream";
    // const char* source = "/dev/video0";  // USB摄像头
    // const char* source = "video.mp4";    // 视频文件

    CRKitStatus status = crkit_open_stream(&stream, source);
    if (status != CRKIT_SUCCESS) {
        printf("打开视频流失败: %s\n", crkit_get_last_error());
        return;
    }

    // 准备上下文
    StreamContext ctx = {0, fopen("stream_log.txt", "w")};

    // 开始视频流推理 (异步，持续处理)
    status = crkit_infer_stream(model, stream, on_frame_processed, &ctx);

    // 运行一段时间后停止
    sleep(60);  // 运行60秒
    crkit_stop_stream_inference(model);

    // 清理
    fclose(ctx.log_file);
    crkit_close_stream(stream);

    printf("总共处理了 %d 帧\n", ctx.frame_count);
}
```

### 场景5: 模型热更新

```c
#include "crkit_api.h"

void hot_update_example(CRKitModel model) {
    // 正常运行中...

    // 需要更新到新模型
    CRKitModelConfig new_config;
    crkit_create_default_model_config(&new_config);
    new_config.model_path = "/path/to/new_model.onnx";
    new_config.input_width = 640;
    new_config.input_height = 640;
    new_config.num_classes = 80;
    new_config.conf_threshold = 0.6;  // 新的阈值
    new_config.nms_threshold = 0.45;

    // 执行热更新 (内部会等待当前推理完成，原子切换)
    CRKitStatus status = crkit_hot_update_model(model, &new_config);

    if (status == CRKIT_SUCCESS) {
        printf("模型热更新成功\n");
        // 继续使用同一个model句柄，已经是新模型了
    } else {
        printf("模型热更新失败: %s\n", crkit_get_last_error());
        // 继续使用旧模型
    }
}
```

### 场景6: 多设备推理

```c
#include "crkit_api.h"

void multi_device_example() {
    // GPU推理引擎
    CRKitEngineConfig gpu_config;
    crkit_create_default_engine_config(&gpu_config);
    gpu_config.backend = CRKIT_BACKEND_TENSORRT;
    gpu_config.device = CRKIT_DEVICE_GPU;
    gpu_config.device_id = 0;
    gpu_config.enable_fp16 = true;

    CRKitEngine gpu_engine;
    crkit_create_engine(&gpu_engine, &gpu_config);

    // CPU推理引擎 (备用)
    CRKitEngineConfig cpu_config;
    crkit_create_default_engine_config(&cpu_config);
    cpu_config.backend = CRKIT_BACKEND_ONNXRUNTIME;
    cpu_config.device = CRKIT_DEVICE_CPU;
    cpu_config.num_worker_threads = 4;

    CRKitEngine cpu_engine;
    crkit_create_engine(&cpu_engine, &cpu_config);

    // 加载相同模型到不同设备
    CRKitModelConfig model_config;
    crkit_create_default_model_config(&model_config);
    model_config.model_path = "/path/to/model.onnx";
    // ... 其他配置 ...

    CRKitModel gpu_model, cpu_model;
    crkit_load_model(gpu_engine, &gpu_model, &model_config);
    crkit_load_model(cpu_engine, &cpu_model, &model_config);

    // 根据负载情况选择设备推理
    CRKitImage image;
    // ... 准备图像 ...

    CRKitResult* result;
    bool use_gpu = check_gpu_available();  // 自定义函数

    if (use_gpu) {
        crkit_infer(gpu_model, &image, &result);
    } else {
        crkit_infer(cpu_model, &image, &result);
    }

    // 处理结果...
    crkit_free_result(result);

    // 清理
    crkit_unload_model(gpu_model);
    crkit_unload_model(cpu_model);
    crkit_destroy_engine(gpu_engine);
    crkit_destroy_engine(cpu_engine);
}
```

## 高级特性

### 1. 性能优化

#### 模型预热

```c
// 模型加载后立即预热
crkit_load_model(engine, &model, &model_config);
crkit_warmup_model(model, 10);  // 执行10次预热推理
```

#### 批处理优化

```c
// 设置合适的批处理大小以充分利用GPU
engine_config.max_batch_size = 8;  // 根据GPU显存调整
```

### 2. 错误处理

```c
CRKitStatus status = crkit_infer(model, &image, &result);
if (status != CRKIT_SUCCESS) {
    // 获取详细错误信息
    const char* error = crkit_get_last_error();
    printf("推理失败: %s (错误码: %d)\n", error, status);

    // 根据错误码采取不同措施
    switch (status) {
        case CRKIT_ERROR_OUT_OF_MEMORY:
            // 减小批处理大小或使用CPU推理
            break;
        case CRKIT_ERROR_TIMEOUT:
            // 重试或跳过
            break;
        default:
            // 其他错误处理
            break;
    }
}
```

### 3. 日志管理

```c
// 设置日志级别
crkit_set_log_level("INFO");  // DEBUG, INFO, WARN, ERROR

// 设置日志文件
crkit_set_log_file("/var/log/crkit_inference.log");

// 关闭日志文件，输出到控制台
crkit_set_log_file(NULL);
```

### 4. 设备信息查询

```c
char info_buffer[1024];

// 查询GPU信息
crkit_get_device_info(CRKIT_DEVICE_GPU, 0, info_buffer, sizeof(info_buffer));
printf("GPU Info: %s\n", info_buffer);

// 查询CPU信息
crkit_get_device_info(CRKIT_DEVICE_CPU, 0, info_buffer, sizeof(info_buffer));
printf("CPU Info: %s\n", info_buffer);
```

### 5. 模型信息查询

```c
char info_buffer[2048];
crkit_get_model_info(model, info_buffer, sizeof(info_buffer));
printf("Model Info (JSON):\n%s\n", info_buffer);

// 输出示例:
// {
//   "model_path": "/path/to/model.onnx",
//   "input_shape": [1, 3, 640, 640],
//   "output_shape": [1, 25200, 85],
//   "num_classes": 80,
//   "backend": "TensorRT",
//   "device": "GPU:0"
// }
```

## 最佳实践

### 1. 资源管理

- 总是配对调用创建和销毁函数
- 使用完结果后立即调用 `crkit_free_result()`
- 程序退出前确保所有资源已释放

### 2. 线程安全

- 同一个 `CRKitModel` 可以在多个线程中同时调用推理
- 模型热更新期间推理调用会自动等待
- 异步推理自动线程安全

### 3. 性能优化建议

- 对于实时视频流，使用异步推理 + 批处理
- 优先使用 TensorRT + FP16 在 GPU 上推理
- 合理设置 `conf_threshold` 和 `nms_threshold`
- 输入图像尽量使用原生格式，减少转换

### 4. 错误处理

- 检查所有API返回值
- 使用 `crkit_get_last_error()` 获取详细错误信息
- 在生产环境中启用日志记录

## 编译链接

### CMake 示例

```cmake
find_package(CRKit REQUIRED)

add_executable(my_app main.c)
target_link_libraries(my_app CRKit::crkit)
```

### 直接编译

```bash
gcc -o my_app main.c -I/path/to/crkit/include -L/path/to/crkit/lib -lcrkit
```

## 示例程序

完整示例程序请参考：
- `examples/simple_inference.c` - 基本推理示例
- `examples/batch_inference.c` - 批量推理示例
- `examples/async_inference.c` - 异步推理示例
- `examples/stream_inference.c` - 视频流推理示例
- `examples/hot_update.c` - 模型热更新示例
