#!/bin/bash
# CRKit SDK 快速开始脚本
# 自动检查环境、下载依赖、编译SDK

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

info "=========================================="
info "  CRKit SDK Quick Start"
info "=========================================="
echo ""

# 检查操作系统
if [ ! -f /etc/os-release ]; then
    error "Unsupported operating system"
    exit 1
fi

source /etc/os-release
info "OS: $PRETTY_NAME"

# 检查必要工具
info "Checking required tools..."
MISSING_TOOLS=""

check_tool() {
    if ! command -v $1 &> /dev/null; then
        MISSING_TOOLS="$MISSING_TOOLS $1"
        warn "$1 not found"
        return 1
    else
        info "✓ $1 found"
        return 0
    fi
}

check_tool "cmake"
check_tool "gcc" || check_tool "g++"
check_tool "make"

if [ -n "$MISSING_TOOLS" ]; then
    error "Missing tools:$MISSING_TOOLS"
    info "Install them with:"
    info "  sudo apt install build-essential cmake"
    exit 1
fi

echo ""

# 检查OpenCV
info "Checking OpenCV..."
if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    info "✓ OpenCV $OPENCV_VERSION found"
elif pkg-config --exists opencv; then
    OPENCV_VERSION=$(pkg-config --modversion opencv)
    info "✓ OpenCV $OPENCV_VERSION found"
else
    warn "OpenCV not found"
    info "Install with:"
    info "  sudo apt install libopencv-dev"
    exit 1
fi

echo ""

# 检查CUDA
info "Checking CUDA..."
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep "release" | awk '{print $5}' | cut -d',' -f1)
    info "✓ CUDA $CUDA_VERSION found"
    HAS_CUDA=1
else
    warn "CUDA not found (GPU inference will not be available)"
    HAS_CUDA=0
fi

echo ""

# 检查TensorRT
info "Checking TensorRT..."
if [ -d "/usr/local/TensorRT" ] || [ -f "/usr/include/x86_64-linux-gnu/NvInfer.h" ]; then
    info "✓ TensorRT found"
    HAS_TENSORRT=1
else
    warn "TensorRT not found"
    info "Download from: https://developer.nvidia.com/tensorrt"
    HAS_TENSORRT=0
fi

echo ""

# 确定构建选项
CMAKE_OPTIONS=""
if [ $HAS_TENSORRT -eq 0 ]; then
    CMAKE_OPTIONS="$CMAKE_OPTIONS --no-tensorrt"
fi

# 开始构建
info "Starting build process..."
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
$SCRIPT_DIR/build.sh $CMAKE_OPTIONS

echo ""
info "=========================================="
info "  Quick Start Complete! ✓"
info "=========================================="
echo ""

# 运行测试
if [ -f "build/bin/test_api" ]; then
    info "Running API tests..."
    build/bin/test_api
    echo ""
fi

info "Next steps:"
info "1. Install the SDK:"
info "   cd build && sudo make install"
echo ""
info "2. Export YOLO model:"
info "   python -c \"from ultralytics import YOLO; YOLO('yolo11n.pt').export(format='onnx')\""
echo ""
info "3. Run inference:"
info "   ./build/bin/simple_inference yolo11n.onnx test.jpg"
echo ""
