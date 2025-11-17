# CRKit SDK 构建指南

## 环境准备

### 系统要求

- **操作系统**: Ubuntu 20.04/22.04 (推荐)
- **编译器**: GCC 7.5+ 或 Clang 9.0+
- **CMake**: 3.15 或更高
- **GPU**: NVIDIA RTX 系列 (RTX 5070, RTX 3060+)

### 安装基础依赖

```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装编译工具
sudo apt install -y build-essential cmake git pkg-config

# 安装OpenCV
sudo apt install -y libopencv-dev

# 验证OpenCV版本
pkg-config --modversion opencv4
```

### 安装CUDA (GPU推理必需)

```bash
# 下载CUDA 12.x (或11.x)
wget https://developer.download.nvidia.com/compute/cuda/12.1.0/local_installers/cuda_12.1.0_530.30.02_linux.run

# 安装
sudo sh cuda_12.1.0_530.30.02_linux.run

# 配置环境变量
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# 验证
nvcc --version
nvidia-smi
```

### 安装TensorRT (推荐)

```bash
# 方法1: 从NVIDIA官网下载TensorRT
# https://developer.nvidia.com/tensorrt

# 下载TensorRT 10.x for CUDA 12.x
# 例如: TensorRT-10.0.1.6.Linux.x86_64-gnu.cuda-12.1.tar.gz

# 解压
tar -xzvf TensorRT-10.0.1.6.Linux.x86_64-gnu.cuda-12.1.tar.gz
sudo mv TensorRT-10.0.1.6 /usr/local/TensorRT

# 设置环境变量
echo 'export TENSORRT_ROOT=/usr/local/TensorRT' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/TensorRT/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# 方法2: 使用apt安装 (Ubuntu)
sudo apt install libnvinfer-dev libnvonnxparsers-dev libnvparsers-dev
```

### 安装ONNX Runtime (可选)

```bash
# 下载ONNX Runtime GPU版本
wget https://github.com/microsoft/onnxruntime/releases/download/v1.15.1/onnxruntime-linux-x64-gpu-1.15.1.tgz

# 解压
tar -xzf onnxruntime-linux-x64-gpu-1.15.1.tgz
sudo mv onnxruntime-linux-x64-gpu-1.15.1 /usr/local/onnxruntime

# 设置环境变量
echo 'export ONNXRUNTIME_ROOT=/usr/local/onnxruntime' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/onnxruntime/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

## 编译SDK

### 1. 克隆仓库

```bash
git clone https://github.com/yourusername/crkit_sdk_v2.git
cd crkit_sdk_v2
```

### 2. 创建构建目录

```bash
mkdir build && cd build
```

### 3. 配置CMake

#### 仅TensorRT后端

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCRKIT_ENABLE_TENSORRT=ON \
    -DCRKIT_ENABLE_ONNXRUNTIME=OFF \
    -DCRKIT_BUILD_EXAMPLES=ON \
    -DCRKIT_BUILD_TESTS=ON
```

#### TensorRT + ONNX Runtime

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCRKIT_ENABLE_TENSORRT=ON \
    -DCRKIT_ENABLE_ONNXRUNTIME=ON \
    -DCRKIT_BUILD_EXAMPLES=ON \
    -DCRKIT_BUILD_TESTS=ON \
    -DONNXRUNTIME_ROOT=/usr/local/onnxruntime \
    -DTENSORRT_ROOT=/usr/local/TensorRT
```

#### 仅CPU (ONNX Runtime)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCRKIT_ENABLE_TENSORRT=OFF \
    -DCRKIT_ENABLE_ONNXRUNTIME=ON \
    -DCRKIT_BUILD_EXAMPLES=ON
```

### 4. 编译

```bash
make -j$(nproc)
```

### 5. 安装 (可选)

```bash
sudo make install

# 或安装到自定义目录
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/crkit
make install
```

## 验证安装

### 检查编译产物

```bash
# 库文件
ls -lh lib/libcrkit.so

# 示例程序
ls -lh bin/simple_inference
ls -lh bin/batch_inference
```

### 准备测试模型

```bash
# 导出YOLO11模型
pip install ultralytics

python -c "
from ultralytics import YOLO
model = YOLO('yolo11n.pt')
model.export(format='onnx', opset=11, simplify=True)
"

# 模型文件: yolo11n.onnx
```

### 运行示例

```bash
# 下载测试图像
wget https://ultralytics.com/images/bus.jpg -O test.jpg

# 运行简单推理示例
./bin/simple_inference yolo11n.onnx test.jpg

# 运行批量推理示例
./bin/batch_inference yolo11n.onnx test.jpg test.jpg test.jpg
```

## 常见问题

### 1. CUDA找不到

**问题**: `Could not find CUDA`

**解决**:
```bash
# 确认CUDA路径
ls /usr/local/cuda

# 设置CMake变量
cmake .. -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
```

### 2. TensorRT找不到

**问题**: `Could not find TensorRT`

**解决**:
```bash
# 手动指定TensorRT路径
cmake .. -DTENSORRT_ROOT=/usr/local/TensorRT
```

### 3. OpenCV版本不匹配

**问题**: `OpenCV version mismatch`

**解决**:
```bash
# 从源码编译OpenCV 4.x
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 4. 链接错误

**问题**: 运行时找不到共享库

**解决**:
```bash
# 更新LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# 或更新ldconfig
sudo ldconfig
```

### 5. GPU内存不足

**问题**: `CUDA out of memory`

**解决**:
- 减小批处理大小: `max_batch_size = 1`
- 减小工作空间: `max_workspace_mb = 512`
- 使用FP16: `enable_fp16 = true`

## 性能优化

### 1. 使用FP16

```bash
cmake .. -DCRKIT_ENABLE_FP16=ON
```

### 2. 调整批处理大小

根据GPU显存调整:
- RTX 5070 (12GB): max_batch_size = 8
- RTX 3060 (8GB): max_batch_size = 4

### 3. 模型优化

```python
# 导出时启用优化
model.export(
    format='onnx',
    opset=11,
    simplify=True,
    dynamic=False,  # 固定输入尺寸
    imgsz=640
)
```

### 4. 启用TensorRT引擎缓存

SDK会自动缓存TensorRT引擎到 `*.onnx.trt` 文件，第二次加载会非常快。

## 交叉编译

### 为ARM64编译 (NVIDIA Jetson)

```bash
# 使用交叉编译工具链
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
```

## 调试模式

```bash
# Debug构建
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 启用详细日志
export CRKIT_LOG_LEVEL=DEBUG

# 运行
./bin/simple_inference yolo11n.onnx test.jpg
```

## 卸载

```bash
# 从安装目录删除
sudo rm -rf /usr/local/lib/libcrkit.so
sudo rm -rf /usr/local/include/crkit_api.h
sudo rm -rf /usr/local/lib/cmake/CRKit
```

## 下一步

- 阅读 [API使用指南](API_USAGE.md)
- 查看 [架构设计文档](ARCHITECTURE.md)
- 探索 [示例程序](../examples/)
