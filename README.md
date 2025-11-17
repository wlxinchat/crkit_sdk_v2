# CRKit - 工业质检AI推理SDK

CRKit (Computer Recognition Kit) 是一个高性能、通用的AI推理SDK，专为工业质检等实时AI推理场景设计。

## 特性

- ✅ **多后端支持**: TensorRT、ONNX Runtime、OpenVINO
- ✅ **跨平台**: Linux (主要支持)、Windows (计划支持)
- ✅ **高性能**: GPU加速、FP16/INT8量化、批处理优化
- ✅ **实时推理**: 针对RTX系列GPU优化，支持实时视频流处理
- ✅ **灵活输入**: 图像文件、内存数据、视频流 (RTSP/USB)
- ✅ **异步推理**: 基于线程池的异步推理框架
- ✅ **模型热更新**: 无缝切换模型，不中断服务
- ✅ **易于集成**: 简洁的C API，标准化接口设计
- ✅ **线程安全**: 所有API线程安全，支持多线程并发

## 架构设计

```
应用层 → C API → 核心业务层 → 后端抽象层 → 推理引擎
                   ├─ 推理管理器
                   ├─ 模型管理器
                   ├─ 异步执行器
                   ├─ 批处理器
                   └─ Pipeline处理器
```

详细架构文档: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

## 快速开始

### 系统要求

- **操作系统**: Linux (Ubuntu 20.04/22.04 推荐)
- **编译器**: GCC 7.5+ 或 Clang 9.0+
- **CMake**: 3.15+
- **CUDA**: 11.x 或 12.x (GPU推理)
- **GPU**: NVIDIA RTX系列 (推荐RTX 3060+)

### 依赖库

#### 必需依赖

- OpenCV 4.x

```bash
sudo apt install libopencv-dev
```

#### 可选依赖 (根据需要选择)

**TensorRT** (推荐，性能最佳)

```bash
# 下载并安装TensorRT
# https://developer.nvidia.com/tensorrt
```

**ONNX Runtime**

```bash
# 下载预编译库
wget https://github.com/microsoft/onnxruntime/releases/download/v1.15.1/onnxruntime-linux-x64-gpu-1.15.1.tgz
tar -xzf onnxruntime-linux-x64-gpu-1.15.1.tgz
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
```

### 编译

```bash
# 克隆仓库
git clone https://github.com/yourusername/crkit_sdk_v2.git
cd crkit_sdk_v2

# 创建构建目录
mkdir build && cd build

# 配置 (默认启用TensorRT和ONNX Runtime)
cmake .. \
    -DCRKIT_ENABLE_TENSORRT=ON \
    -DCRKIT_ENABLE_ONNXRUNTIME=ON \
    -DCRKIT_BUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 安装 (可选)
sudo make install
```

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CRKIT_BUILD_SHARED` | ON | 构建动态库 |
| `CRKIT_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `CRKIT_BUILD_TESTS` | ON | 构建测试 |
| `CRKIT_ENABLE_TENSORRT` | ON | 启用TensorRT后端 |
| `CRKIT_ENABLE_ONNXRUNTIME` | ON | 启用ONNX Runtime后端 |
| `CRKIT_ENABLE_OPENVINO` | OFF | 启用OpenVINO后端 |
| `CRKIT_ENABLE_LOGGING` | ON | 启用日志 |

### 快速示例

```c
#include "crkit_api.h"

int main() {
    // 1. 创建引擎
    CRKitEngineConfig config;
    crkit_create_default_engine_config(&config);
    config.backend = CRKIT_BACKEND_TENSORRT;
    config.device = CRKIT_DEVICE_GPU;
    config.device_id = 0;

    CRKitEngine engine;
    crkit_create_engine(&engine, &config);

    // 2. 加载模型
    CRKitModelConfig model_config;
    crkit_create_default_model_config(&model_config);
    model_config.model_path = "model.onnx";
    model_config.input_width = 640;
    model_config.input_height = 640;
    model_config.num_classes = 80;
    model_config.conf_threshold = 0.5;
    model_config.nms_threshold = 0.45;

    CRKitModel model;
    crkit_load_model(engine, &model, &model_config);

    // 3. 推理
    CRKitResult* result;
    crkit_infer_from_file(model, "test.jpg", &result);

    // 4. 处理结果
    printf("检测到 %d 个目标\n", result->num_detections);
    for (int i = 0; i < result->num_detections; i++) {
        CRKitDetection* det = &result->detections[i];
        printf("类别: %s, 置信度: %.2f\n",
               det->class_name, det->confidence);
    }

    // 5. 清理
    crkit_free_result(result);
    crkit_unload_model(model);
    crkit_destroy_engine(engine);

    return 0;
}
```

编译示例:

```bash
gcc -o demo demo.c -I/usr/local/include -L/usr/local/lib -lcrkit -lopencv_core -lopencv_imgcodecs
./demo
```

## 文档

- [API使用指南](docs/API_USAGE.md) - 完整的API使用说明和示例
- [架构设计](docs/ARCHITECTURE.md) - 详细的系统架构设计
- [性能调优](docs/PERFORMANCE.md) - 性能优化指南 (TODO)
- [FAQ](docs/FAQ.md) - 常见问题解答 (TODO)

## 示例程序

在 `examples/` 目录下提供了多个完整示例:

- `simple_inference.c` - 单图像推理
- `batch_inference.c` - 批量推理
- `async_inference.c` - 异步推理
- `stream_inference.c` - 视频流推理
- `hot_update.c` - 模型热更新

## 性能基准

基于RTX 5070 GPU (TensorRT FP16):

| 模型 | 输入尺寸 | 批次大小 | FPS | 延迟(ms) |
|------|----------|----------|-----|----------|
| YOLO11n | 640x640 | 1 | 350+ | 2.8 |
| YOLO11s | 640x640 | 1 | 280+ | 3.5 |
| YOLO11m | 640x640 | 1 | 180+ | 5.5 |
| YOLO11n | 640x640 | 4 | 800+ | 5.0 (batch) |

*测试环境: Ubuntu 22.04, CUDA 12.1, TensorRT 10.x*

## 模型准备

### 从YOLO训练模型到ONNX

```bash
# 导出ONNX模型
yolo export model=yolo11n.pt format=onnx opset=11 simplify=True

# 验证ONNX模型
python -c "import onnx; onnx.checker.check_model('yolo11n.onnx')"
```

### TensorRT引擎生成 (可选)

SDK会自动将ONNX转换为TensorRT引擎并缓存，也可以手动转换:

```bash
trtexec --onnx=yolo11n.onnx \
        --saveEngine=yolo11n.trt \
        --fp16 \
        --workspace=4096 \
        --minShapes=images:1x3x640x640 \
        --optShapes=images:1x3x640x640 \
        --maxShapes=images:4x3x640x640
```

## 项目结构

```
crkit_sdk_v2/
├── CMakeLists.txt          # 主CMake配置
├── README.md               # 本文件
├── include/
│   └── crkit_api.h         # 公开C API头文件
├── src/
│   ├── api/                # API实现
│   ├── core/               # 核心业务逻辑
│   ├── backend/            # 推理后端实现
│   └── utils/              # 工具类
├── examples/               # 示例程序
├── tests/                  # 单元测试
├── docs/                   # 文档
├── cmake/                  # CMake模块
└── third_party/            # 第三方库
```

## 开发指南

### 添加新的推理后端

1. 继承 `IInferenceBackend` 接口
2. 实现必要的虚函数
3. 在工厂类中注册新后端
4. 更新CMake配置

详见: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) (TODO)

### 自定义预处理/后处理

SDK提供插件机制，可以注册自定义处理函数。详见API文档。

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

## 贡献

欢迎贡献! 请查看 [CONTRIBUTING.md](CONTRIBUTING.md) (TODO)

## 联系方式

- 问题反馈: [GitHub Issues](https://github.com/yourusername/crkit_sdk_v2/issues)
- 邮箱: wlxinchat@gmail.com

## 致谢

- NVIDIA TensorRT
- ONNX Runtime
- OpenCV
- Ultralytics YOLO

## 更新日志

### v1.0.0 (2025-01-XX)

- ✅ 初始版本发布
- ✅ 支持TensorRT和ONNX Runtime
- ✅ 目标检测支持
- ✅ 同步/异步推理
- ✅ 批处理和模型热更新
