#!/bin/bash
# CRKit SDK 构建脚本

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的消息
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 默认配置
BUILD_TYPE="Release"
ENABLE_TENSORRT="ON"
ENABLE_ONNXRUNTIME="ON"
BUILD_EXAMPLES="ON"
BUILD_TESTS="ON"
CLEAN_BUILD="OFF"
INSTALL_PREFIX="/usr/local"
NUM_JOBS=$(nproc)

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-tensorrt)
            ENABLE_TENSORRT="OFF"
            shift
            ;;
        --no-onnx)
            ENABLE_ONNXRUNTIME="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --clean)
            CLEAN_BUILD="ON"
            shift
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --jobs)
            NUM_JOBS="$2"
            shift 2
            ;;
        --help)
            echo "CRKit SDK Build Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug              Build in Debug mode (default: Release)"
            echo "  --no-tensorrt        Disable TensorRT backend"
            echo "  --no-onnx            Disable ONNX Runtime backend"
            echo "  --no-examples        Don't build examples"
            echo "  --no-tests           Don't build tests"
            echo "  --clean              Clean build directory before building"
            echo "  --prefix PATH        Installation prefix (default: /usr/local)"
            echo "  --jobs N             Number of parallel jobs (default: nproc)"
            echo "  --help               Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                         # Build with default settings"
            echo "  $0 --debug --clean         # Clean debug build"
            echo "  $0 --no-tensorrt           # Build without TensorRT"
            echo "  $0 --prefix /opt/crkit     # Install to /opt/crkit"
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

info "=========================================="
info "  CRKit SDK Build Configuration"
info "=========================================="
info "Build Type: $BUILD_TYPE"
info "TensorRT: $ENABLE_TENSORRT"
info "ONNX Runtime: $ENABLE_ONNXRUNTIME"
info "Examples: $BUILD_EXAMPLES"
info "Tests: $BUILD_TESTS"
info "Install Prefix: $INSTALL_PREFIX"
info "Parallel Jobs: $NUM_JOBS"
info "=========================================="
echo ""

# 清理构建目录
if [ "$CLEAN_BUILD" == "ON" ] && [ -d "build" ]; then
    warn "Cleaning build directory..."
    rm -rf build
fi

# 创建构建目录
mkdir -p build
cd build

# 运行CMake
info "Running CMake configuration..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCRKIT_ENABLE_TENSORRT=$ENABLE_TENSORRT \
    -DCRKIT_ENABLE_ONNXRUNTIME=$ENABLE_ONNXRUNTIME \
    -DCRKIT_BUILD_EXAMPLES=$BUILD_EXAMPLES \
    -DCRKIT_BUILD_TESTS=$BUILD_TESTS \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX

if [ $? -ne 0 ]; then
    error "CMake configuration failed"
    exit 1
fi

echo ""
info "Building CRKit SDK..."
make -j$NUM_JOBS

if [ $? -ne 0 ]; then
    error "Build failed"
    exit 1
fi

echo ""
info "=========================================="
info "  Build Successful! ✓"
info "=========================================="
echo ""
info "Build artifacts:"
info "  Library: $(pwd)/lib/libcrkit.so"
if [ "$BUILD_EXAMPLES" == "ON" ]; then
    info "  Examples: $(pwd)/bin/"
fi
if [ "$BUILD_TESTS" == "ON" ]; then
    info "  Tests: $(pwd)/bin/"
fi
echo ""
info "To install the SDK:"
info "  sudo make install"
echo ""
info "To run tests:"
info "  cd build && ctest"
echo ""
