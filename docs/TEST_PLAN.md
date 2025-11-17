# CRKit SDK 测试计划

**版本**: 1.0.0
**日期**: 2025-01-17
**测试负责人**: wlxinchat@gmail.com

## 1. 测试目标

验证CRKit SDK的功能完整性、稳定性、性能和可交付性，确保所有Bug修复有效且不引入新问题。

## 2. 测试范围

### 2.1 代码层级测试
- 静态代码分析
- 编译测试
- 单元测试
- 集成测试
- 性能测试

### 2.2 功能模块测试
- API接口测试
- 核心功能测试
- 后端引擎测试
- 工具类测试

## 3. 测试用例设计

### TC-001: 静态代码分析
**优先级**: P0
**目的**: 验证代码语法正确性和潜在问题

**测试步骤**:
1. 使用C++编译器检查语法错误
2. 检查头文件依赖关系
3. 验证代码风格一致性

**预期结果**:
- 无语法错误
- 所有头文件可以正确包含
- 代码符合C++11标准

---

### TC-002: 编译测试
**优先级**: P0
**目的**: 验证项目可以成功编译

**测试步骤**:
1. 配置CMake（不依赖外部库）
2. 编译核心源代码
3. 检查编译警告和错误

**预期结果**:
- 编译成功，无错误
- 警告数量为0或在可接受范围内
- 生成库文件

---

### TC-003: Detection构造函数测试
**优先级**: P0
**目的**: 验证Bug修复 - Detection构造函数正确初始化

**测试代码**:
```cpp
Detection det(10.0f, 20.0f, 30.0f, 40.0f, 1, 0.95f);
assert(det.x == 10.0f);
assert(det.y == 20.0f);
assert(det.width == 30.0f);   // Bug修复验证
assert(det.height == 40.0f);
assert(det.class_id == 1);
assert(det.confidence == 0.95f);
```

**预期结果**:
- 所有成员变量正确初始化
- width = 30.0（之前Bug会导致未定义值）

---

### TC-004: CUDA错误检查宏测试
**优先级**: P0
**目的**: 验证CUDA_CHECK宏正确处理错误

**测试步骤**:
1. 检查CUDA_CHECK宏定义
2. 验证所有CUDA API调用都使用CUDA_CHECK
3. 模拟CUDA错误，验证错误处理

**预期结果**:
- CUDA_CHECK宏正确定义
- 所有关键CUDA调用都有错误检查
- 错误时返回CRKIT_ERROR_DEVICE_ERROR

---

### TC-005: Logger线程安全测试
**优先级**: P1
**目的**: 验证Logger在多线程环境下的安全性

**测试代码**:
```cpp
#include <thread>
#include <vector>

void log_thread(int id) {
    for (int i = 0; i < 100; i++) {
        LOG_INFO("Thread %d: message %d", id, i);
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(log_thread, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
```

**预期结果**:
- 无竞态条件
- 无崩溃
- 时间戳正确
- 无数据损坏

---

### TC-006: 预处理归一化性能测试
**优先级**: P1
**目的**: 验证归一化优化效果

**测试代码**:
```cpp
cv::Mat img(640, 640, CV_32FC3);
Preprocessor prep;
prep.setMean({0.485, 0.456, 0.406});
prep.setStd({0.229, 0.224, 0.225});

auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 1000; i++) {
    cv::Mat img_copy = img.clone();
    prep.normalize(img_copy);
}
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

// 预期优化后 < 100ms (1000次)
assert(duration.count() < 100);
```

**预期结果**:
- 1000次归一化 < 100ms
- 结果数值正确
- 无内存泄漏

---

### TC-007: API基本功能测试
**优先级**: P0
**目的**: 验证C API接口可用性

**测试步骤**:
1. 创建引擎配置
2. 创建引擎实例
3. 销毁引擎
4. 检查返回状态

**测试代码**:
```c
CRKitEngineConfig config;
crkit_create_default_engine_config(&config);
assert(config.backend == CRKIT_BACKEND_TENSORRT);
assert(config.device == CRKIT_DEVICE_GPU);

CRKitEngine engine;
// 注意: 实际创建需要GPU环境
// CRKitStatus status = crkit_create_engine(&engine, &config);
// assert(status == CRKIT_SUCCESS);
```

**预期结果**:
- 配置创建成功
- 默认值正确
- API调用无崩溃

---

### TC-008: 内存管理测试
**优先级**: P0
**目的**: 验证无内存泄漏

**测试步骤**:
1. 创建和销毁引擎多次
2. 加载和卸载模型多次
3. 执行推理并释放结果
4. 使用valgrind检测内存泄漏

**预期结果**:
- 无内存泄漏
- 无重复释放
- 无使用已释放的内存

---

### TC-009: TensorRT后端测试
**优先级**: P1
**目的**: 验证TensorRT后端功能

**前提条件**:
- CUDA环境
- TensorRT库
- 有效的ONNX/TRT模型

**测试步骤**:
1. 创建TensorRT后端
2. 加载ONNX模型
3. 执行推理
4. 验证输出

**预期结果**:
- 模型加载成功
- 推理正常执行
- 输出张量正确

---

### TC-010: ONNX Runtime后端测试
**优先级**: P1
**目的**: 验证ONNX Runtime后端功能

**前提条件**:
- ONNX Runtime库
- 有效的ONNX模型

**测试步骤**:
1. 创建ONNX Runtime后端
2. 加载ONNX模型
3. 执行CPU推理
4. 执行GPU推理（如果支持）

**预期结果**:
- CPU推理成功
- GPU推理成功（如果有CUDA）
- 输出正确

---

### TC-011: 图像预处理流程测试
**优先级**: P1
**目的**: 验证完整预处理流程

**测试步骤**:
1. 加载测试图像
2. Resize到目标尺寸
3. Padding（letterbox）
4. 归一化
5. BGR转RGB
6. HWC转CHW

**预期结果**:
- 输出尺寸正确 (C, H, W)
- 数值范围正确
- 保持宽高比

---

### TC-012: 目标检测后处理测试
**优先级**: P1
**目的**: 验证NMS和坐标映射

**测试数据**:
```cpp
// 模拟检测输出
std::vector<Detection> detections = {
    {100, 100, 50, 50, 0, 0.9},   // 高置信度
    {105, 105, 48, 48, 0, 0.85},  // 重叠框，应被NMS过滤
    {300, 300, 60, 60, 1, 0.7},   // 不同类别
    {100, 100, 50, 50, 0, 0.4}    // 低置信度，应被过滤
};
```

**预期结果**:
- 置信度过滤正确（conf < 0.5 被过滤）
- NMS去重正确（IoU > threshold被过滤）
- 坐标映射回原图正确

---

### TC-013: 批处理推理测试
**优先级**: P2
**目的**: 验证批量推理功能

**测试步骤**:
1. 准备4张测试图像
2. 批量推理
3. 验证每张图像的结果

**预期结果**:
- 批处理正常执行
- 每张图像结果独立正确
- 性能优于单张推理4次

---

### TC-014: 异步推理测试
**优先级**: P2
**目的**: 验证异步推理功能

**测试步骤**:
1. 提交异步推理任务
2. 继续执行其他操作
3. 等待推理完成
4. 获取结果

**预期结果**:
- 异步提交成功
- 不阻塞主线程
- 结果正确

---

### TC-015: 模型热更新测试
**优先级**: P2
**目的**: 验证运行时更换模型

**测试步骤**:
1. 加载模型A
2. 执行推理
3. 热更新为模型B
4. 执行推理
5. 验证使用的是模型B

**预期结果**:
- 更新过程无服务中断
- 新模型立即生效
- 无内存泄漏

---

### TC-016: 错误处理测试
**优先级**: P1
**目的**: 验证错误处理机制

**测试场景**:
1. 传入NULL指针
2. 无效的模型路径
3. 不支持的后端类型
4. 内存不足
5. 无效的图像数据

**预期结果**:
- 返回正确的错误码
- 不崩溃
- 错误信息清晰

---

### TC-017: 线程安全测试
**优先级**: P1
**目的**: 验证多线程并发推理

**测试代码**:
```cpp
void inference_thread(CRKitModel model, const char* image) {
    CRKitResult* result;
    CRKitStatus status = crkit_infer_from_file(model, image, &result);
    assert(status == CRKIT_SUCCESS);
    crkit_free_result(result);
}

// 启动10个线程同时推理
std::vector<std::thread> threads;
for (int i = 0; i < 10; i++) {
    threads.emplace_back(inference_thread, model, "test.jpg");
}
```

**预期结果**:
- 无竞态条件
- 无死锁
- 每个线程结果正确

---

### TC-018: 性能基准测试
**优先级**: P2
**目的**: 验证性能指标

**测试环境**: RTX 5070 GPU

**测试指标**:
| 模型 | 批次 | 目标FPS | 目标延迟 |
|------|------|---------|----------|
| YOLO11n | 1 | >300 | <4ms |
| YOLO11s | 1 | >250 | <5ms |
| YOLO11m | 1 | >150 | <7ms |
| YOLO11n | 4 | >700 | <6ms(batch) |

**预期结果**:
- 达到或超过目标性能
- 性能稳定（方差小）

---

### TC-019: 内存占用测试
**优先级**: P2
**目的**: 验证内存使用合理

**测试步骤**:
1. 记录初始内存
2. 加载模型
3. 执行1000次推理
4. 卸载模型
5. 检查内存泄漏

**预期结果**:
- 无内存泄漏
- 峰值内存在合理范围
- 模型卸载后内存释放

---

### TC-020: 长时间运行稳定性测试
**优先级**: P2
**目的**: 验证长时间运行稳定性

**测试步骤**:
1. 连续推理10000次
2. 监控内存、GPU使用率
3. 检查崩溃或异常

**预期结果**:
- 无崩溃
- 无性能衰减
- 无内存泄漏

---

## 4. 测试环境

### 4.1 最小测试环境（语法检查）
- GCC/Clang编译器
- CMake 3.15+

### 4.2 完整测试环境
- Ubuntu 20.04/22.04
- GCC 9.0+ 或 Clang 10.0+
- CMake 3.15+
- CUDA 11.x/12.x
- TensorRT 8.x/10.x
- ONNX Runtime 1.15+
- OpenCV 4.x
- NVIDIA RTX GPU (推荐RTX 3060+)

### 4.3 测试工具
- valgrind (内存检查)
- gdb (调试)
- perf/nsys (性能分析)
- cppcheck (静态分析)

## 5. 测试执行计划

### 阶段1: 静态分析 (优先级: P0)
- [ ] TC-001: 静态代码分析
- [ ] TC-002: 编译测试

### 阶段2: 单元测试 (优先级: P0-P1)
- [ ] TC-003: Detection构造函数测试
- [ ] TC-004: CUDA错误检查测试
- [ ] TC-005: Logger线程安全测试
- [ ] TC-006: 预处理性能测试
- [ ] TC-007: API基本功能测试
- [ ] TC-008: 内存管理测试

### 阶段3: 功能测试 (优先级: P1-P2)
- [ ] TC-009: TensorRT后端测试
- [ ] TC-010: ONNX Runtime后端测试
- [ ] TC-011: 图像预处理测试
- [ ] TC-012: 后处理测试
- [ ] TC-016: 错误处理测试

### 阶段4: 高级功能测试 (优先级: P2)
- [ ] TC-013: 批处理测试
- [ ] TC-014: 异步推理测试
- [ ] TC-015: 模型热更新测试
- [ ] TC-017: 线程安全测试

### 阶段5: 性能和稳定性测试 (优先级: P2)
- [ ] TC-018: 性能基准测试
- [ ] TC-019: 内存占用测试
- [ ] TC-020: 长时间运行测试

## 6. 通过标准

### 6.1 P0级别测试
- 所有P0测试用例必须100%通过
- 无编译错误
- 无严重Bug

### 6.2 P1级别测试
- P1测试用例通过率 ≥ 95%
- 已知问题有解决方案

### 6.3 P2级别测试
- P2测试用例通过率 ≥ 90%
- 性能达到目标的80%以上

## 7. 风险评估

### 7.1 环境依赖风险
- 测试环境可能缺少GPU硬件
- 外部库版本兼容性问题

### 7.2 缓解措施
- 提供模拟测试（mock）
- 分层测试（不依赖GPU的优先）
- 详细文档说明依赖

## 8. 测试报告

测试完成后将生成:
- 测试执行报告
- Bug列表
- 性能基准数据
- 建议改进项

---

**文档版本**: 1.0
**最后更新**: 2025-01-17
