cmake_minimum_required(VERSION 3.15)

project(CRKit VERSION 1.0.0 LANGUAGES CXX C)

# C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C11标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Werror
        -fvisibility=hidden
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# 选项
option(CRKIT_BUILD_SHARED "Build shared library" ON)
option(CRKIT_BUILD_EXAMPLES "Build examples" ON)
option(CRKIT_BUILD_TESTS "Build tests" ON)
option(CRKIT_ENABLE_TENSORRT "Enable TensorRT backend" ON)
option(CRKIT_ENABLE_ONNXRUNTIME "Enable ONNX Runtime backend" ON)
option(CRKIT_ENABLE_OPENVINO "Enable OpenVINO backend" OFF)
option(CRKIT_ENABLE_LOGGING "Enable logging" ON)

# 包含路径
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# CMake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 查找依赖
find_package(Threads REQUIRED)
find_package(OpenCV 4 REQUIRED)

# CUDA和TensorRT
if(CRKIT_ENABLE_TENSORRT)
    find_package(CUDA REQUIRED)
    find_package(TensorRT REQUIRED)
    add_definitions(-DCRKIT_ENABLE_TENSORRT)
endif()

# ONNX Runtime
if(CRKIT_ENABLE_ONNXRUNTIME)
    find_package(ONNXRuntime REQUIRED)
    add_definitions(-DCRKIT_ENABLE_ONNXRUNTIME)
endif()

# OpenVINO
if(CRKIT_ENABLE_OPENVINO)
    find_package(OpenVINO REQUIRED)
    add_definitions(-DCRKIT_ENABLE_OPENVINO)
endif()

# 日志
if(CRKIT_ENABLE_LOGGING)
    add_definitions(-DCRKIT_ENABLE_LOGGING)
endif()

# 源文件
set(CRKIT_CORE_SOURCES
    src/api/crkit_api.cpp
    src/core/preprocessor.cpp
    src/core/postprocessor.cpp
    src/backend/backend_factory.cpp
    src/utils/logger.cpp
    src/utils/error.cpp
)

# 后端源文件
set(CRKIT_BACKEND_SOURCES)

if(CRKIT_ENABLE_TENSORRT)
    list(APPEND CRKIT_BACKEND_SOURCES
        src/backend/tensorrt_backend.cpp
    )
endif()

if(CRKIT_ENABLE_ONNXRUNTIME)
    list(APPEND CRKIT_BACKEND_SOURCES
        src/backend/onnxruntime_backend.cpp
    )
endif()

if(CRKIT_ENABLE_OPENVINO)
    list(APPEND CRKIT_BACKEND_SOURCES
        src/backend/openvino_backend.cpp
    )
endif()

# 创建库
if(CRKIT_BUILD_SHARED)
    add_library(crkit SHARED
        ${CRKIT_CORE_SOURCES}
        ${CRKIT_BACKEND_SOURCES}
    )
    target_compile_definitions(crkit PRIVATE CRKIT_BUILD_SHARED)
else()
    add_library(crkit STATIC
        ${CRKIT_CORE_SOURCES}
        ${CRKIT_BACKEND_SOURCES}
    )
endif()

# 设置别名
add_library(CRKit::crkit ALIAS crkit)

# 链接库
target_link_libraries(crkit
    PUBLIC
        Threads::Threads
        ${OpenCV_LIBS}
    PRIVATE
)

# TensorRT链接
if(CRKIT_ENABLE_TENSORRT)
    target_include_directories(crkit PRIVATE ${CUDA_INCLUDE_DIRS} ${TensorRT_INCLUDE_DIRS})
    target_link_libraries(crkit PRIVATE
        ${CUDA_LIBRARIES}
        ${TensorRT_LIBRARIES}
        cudart
        nvinfer
        nvonnxparser
    )
endif()

# ONNX Runtime链接
if(CRKIT_ENABLE_ONNXRUNTIME)
    target_include_directories(crkit PRIVATE ${ONNXRuntime_INCLUDE_DIRS})
    target_link_libraries(crkit PRIVATE ${ONNXRuntime_LIBRARIES})
endif()

# OpenVINO链接
if(CRKIT_ENABLE_OPENVINO)
    target_link_libraries(crkit PRIVATE openvino::runtime)
endif()

# 设置输出目录
set_target_properties(crkit PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "crkit"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 安装规则
install(TARGETS crkit
    EXPORT CRKitTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT CRKitTargets
    FILE CRKitTargets.cmake
    NAMESPACE CRKit::
    DESTINATION lib/cmake/CRKit
)

# 生成配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/CRKitConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/CRKitConfig.cmake"
    INSTALL_DESTINATION lib/cmake/CRKit
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/CRKitConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/CRKitConfig.cmake"
    "${CMAKE_BINARY_DIR}/CRKitConfigVersion.cmake"
    DESTINATION lib/cmake/CRKit
)

# 示例
if(CRKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试
if(CRKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 打印配置信息
message(STATUS "==================== CRKit Configuration ====================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build shared library: ${CRKIT_BUILD_SHARED}")
message(STATUS "Build examples: ${CRKIT_BUILD_EXAMPLES}")
message(STATUS "Build tests: ${CRKIT_BUILD_TESTS}")
message(STATUS "Enable TensorRT: ${CRKIT_ENABLE_TENSORRT}")
message(STATUS "Enable ONNX Runtime: ${CRKIT_ENABLE_ONNXRUNTIME}")
message(STATUS "Enable OpenVINO: ${CRKIT_ENABLE_OPENVINO}")
message(STATUS "Enable logging: ${CRKIT_ENABLE_LOGGING}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
if(CRKIT_ENABLE_TENSORRT)
    message(STATUS "CUDA version: ${CUDA_VERSION}")
    message(STATUS "TensorRT version: ${TensorRT_VERSION}")
endif()
message(STATUS "=============================================================")
