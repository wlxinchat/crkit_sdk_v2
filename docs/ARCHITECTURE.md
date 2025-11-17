# AI推理SDK架构设计文档

## 1. 架构概述

本SDK为工业质检场景设计的通用AI推理框架，采用分层架构，支持多种推理后端和实时处理需求。

### 1.1 设计原则

- **抽象化**: 推理引擎无关，屏蔽底层实现细节
- **标准化**: 提供统一的C/C++ API接口
- **高性能**: 支持批处理、异步推理、GPU加速
- **灵活性**: 支持模型热更新、多种输入源
- **可扩展**: 易于添加新的推理后端

## 2. 分层架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (Application)                   │
│            用户代码通过标准C API接口调用                   │
└─────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────┐
│                  API接口层 (API Layer)                    │
│     - C API (crkit_api.h)                               │
│     - 对象生命周期管理                                     │
│     - 错误处理                                            │
└─────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────┐
│                核心业务层 (Core Layer)                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ 推理管理器    │  │ 模型管理器    │  │ 异步队列     │  │
│  │InferenceEngine│  │ModelManager  │  │AsyncExecutor │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ 输入处理器    │  │ Pipeline     │  │ 批处理管理    │  │
│  │InputProcessor│  │PrePostProcess│  │BatchProcessor│  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────┐
│              推理后端抽象层 (Backend Abstraction)          │
│                   IInferenceBackend                      │
│              统一的推理引擎接口定义                         │
└─────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
┌───────────────┐  ┌────────────────┐  ┌──────────────┐
│  TensorRT     │  │  ONNX Runtime  │  │   OpenVINO   │
│   Backend     │  │    Backend     │  │   Backend    │
└───────────────┘  └────────────────┘  └──────────────┘
```

## 3. 核心组件设计

### 3.1 推理引擎 (InferenceEngine)

**职责**:
- 管理推理后端实例
- 协调各组件完成推理流程
- 支持同步/异步推理
- 批处理调度

**关键接口**:
- `Initialize()`: 初始化引擎
- `LoadModel()`: 加载模型
- `Infer()`: 执行推理
- `InferAsync()`: 异步推理
- `HotUpdate()`: 模型热更新

### 3.2 推理后端接口 (IInferenceBackend)

**职责**: 定义统一的推理引擎抽象接口

**关键接口**:
```cpp
class IInferenceBackend {
    virtual Status LoadModel(const ModelConfig& config) = 0;
    virtual Status Infer(const Tensor& input, Tensor& output) = 0;
    virtual Status InferBatch(const std::vector<Tensor>& inputs,
                             std::vector<Tensor>& outputs) = 0;
    virtual DeviceType GetDeviceType() = 0;
};
```

### 3.3 模型管理器 (ModelManager)

**职责**:
- 模型加载/卸载
- 模型热更新（原子切换）
- 模型元数据管理
- 多版本管理

**特性**:
- 使用双缓冲机制实现无缝切换
- 引用计数保证正在使用的模型不被卸载
- 支持模型预热

### 3.4 输入处理器 (InputProcessor)

**职责**:
- 统一图像/视频流输入接口
- 支持多种图像格式 (RGB, BGR, YUV等)
- 视频流解码和帧提取
- 数据格式转换

**支持的输入源**:
- 文件路径 (图像文件)
- 内存缓冲区 (raw data)
- RTSP/USB视频流
- OpenCV Mat对象

### 3.5 预处理/后处理Pipeline

**预处理Pipeline**:
- Resize (保持宽高比/拉伸/填充)
- Normalization (均值/方差归一化)
- 颜色空间转换
- 数据排布转换 (HWC->CHW)
- 批次拼接

**后处理Pipeline (目标检测)**:
- Bounding Box解码
- NMS (非极大值抑制)
- 置信度过滤
- 坐标映射回原图
- 结果排序

### 3.6 异步执行器 (AsyncExecutor)

**职责**:
- 管理推理任务队列
- 线程池管理
- 回调机制
- 超时控制

**特性**:
- 基于线程池的任务调度
- 支持优先级队列
- Future/Promise模式

### 3.7 批处理管理器 (BatchProcessor)

**职责**:
- 动态批次组装
- 批次大小优化
- 超时控制（避免饥饿）

**策略**:
- 固定批次大小
- 动态批次（时间窗口+最大批次）

## 4. 数据结构设计

### 4.1 核心数据类型

```cpp
// 张量表示
struct Tensor {
    void* data;
    int dims[4];  // NCHW
    DataType dtype;
    DeviceType device;
};

// 检测结果
struct DetectionResult {
    float x, y, width, height;  // bbox
    int class_id;
    float confidence;
    const char* class_name;
};

// 推理结果
struct InferenceResult {
    DetectionResult* detections;
    int num_detections;
    float inference_time_ms;
    int64_t timestamp;
};
```

### 4.2 配置结构

```cpp
// 引擎配置
struct EngineConfig {
    BackendType backend;      // TensorRT, ONNXRuntime
    DeviceType device;        // CPU, GPU
    int device_id;
    int max_batch_size;
    bool enable_fp16;
    int num_worker_threads;
};

// 模型配置
struct ModelConfig {
    const char* model_path;
    int input_width;
    int input_height;
    int num_classes;
    float conf_threshold;
    float nms_threshold;
};
```

## 5. 关键技术特性

### 5.1 模型热更新机制

```
1. 加载新模型到临时内存
2. 验证模型有效性
3. 等待当前推理任务完成
4. 原子替换模型指针（双缓冲）
5. 释放旧模型资源
```

### 5.2 异步推理流程

```
用户提交任务 -> 任务队列 -> 工作线程 -> 推理执行 -> 回调通知
              ↑                                         ↓
              └─────────── 线程池管理 ────────────────┘
```

### 5.3 批处理优化

```
Strategy 1: 固定时间窗口 (10ms) 收集请求，批量推理
Strategy 2: 达到max_batch_size立即推理
Strategy 3: 混合策略（时间窗口 + 批次大小双触发）
```

## 6. 性能优化策略

### 6.1 内存优化
- 内存池管理，减少动态分配
- GPU显存预分配
- 零拷贝技术（输入输出）

### 6.2 计算优化
- TensorRT INT8/FP16量化
- CUDA Stream并行
- 算子融合

### 6.3 流水线优化
- 预处理/推理/后处理并行
- 多CUDA Stream重叠执行

## 7. 线程安全设计

- API层接口线程安全（互斥锁保护）
- 模型热更新使用读写锁
- 异步队列使用无锁队列或条件变量
- 批处理器使用原子操作

## 8. 错误处理

### 8.1 错误码设计

```cpp
enum StatusCode {
    SUCCESS = 0,
    ERROR_INVALID_PARAM = -1,
    ERROR_MODEL_LOAD_FAILED = -2,
    ERROR_INFERENCE_FAILED = -3,
    ERROR_OUT_OF_MEMORY = -4,
    ERROR_TIMEOUT = -5,
    ERROR_NOT_INITIALIZED = -6
};
```

### 8.2 错误处理策略
- 所有API返回状态码
- 提供错误信息查询接口
- 日志记录（分级：DEBUG/INFO/WARN/ERROR）

## 9. 扩展性设计

### 9.1 新增推理后端
继承`IInferenceBackend`接口，实现具体后端逻辑，注册到工厂类。

### 9.2 自定义预处理/后处理
提供插件机制，用户可注册自定义处理函数。

### 9.3 多任务类型支持
当前针对目标检测，未来可扩展分类、分割等任务。

## 10. 部署架构

```
应用程序
    │
    ├─ libcrkit_sdk.so (核心SDK)
    ├─ libcrkit_tensorrt.so (TensorRT后端)
    ├─ libcrkit_onnx.so (ONNX Runtime后端)
    └─ 配置文件/模型文件
```

## 11. 依赖库

- **必需**:
  - C++14 或更高
  - CMake 3.15+
  - OpenCV 4.x (输入处理)

- **可选** (按需链接):
  - TensorRT 8.x/10.x
  - ONNX Runtime 1.15+
  - CUDA 11.x/12.x
  - cuDNN 8.x

## 12. 交付标准

### 12.1 功能标准
- ✓ 支持TensorRT/ONNX Runtime
- ✓ 支持CPU/GPU推理
- ✓ 支持同步/异步推理
- ✓ 支持批处理推理
- ✓ 支持模型热更新
- ✓ 支持图像/视频流输入
- ✓ 实时性能 (RTX 5070: >30 FPS)

### 12.2 接口标准
- ✓ 简洁的C API
- ✓ 完整的错误处理
- ✓ 线程安全
- ✓ 资源自动管理

### 12.3 质量标准
- ✓ 单元测试覆盖率 >80%
- ✓ 内存泄漏检测 (valgrind)
- ✓ 性能基准测试
- ✓ 完整的API文档
