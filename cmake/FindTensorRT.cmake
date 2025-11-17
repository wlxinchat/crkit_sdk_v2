# FindTensorRT.cmake
# 查找TensorRT库

find_path(TensorRT_INCLUDE_DIR
    NAMES NvInfer.h
    HINTS
        ${TENSORRT_ROOT}
        $ENV{TENSORRT_ROOT}
        /usr/include/x86_64-linux-gnu
        /usr/local/cuda/include
    PATH_SUFFIXES include
)

find_library(TensorRT_nvinfer_LIBRARY
    NAMES nvinfer
    HINTS
        ${TENSORRT_ROOT}
        $ENV{TENSORRT_ROOT}
        /usr/lib/x86_64-linux-gnu
        /usr/local/cuda/lib64
    PATH_SUFFIXES lib lib64
)

find_library(TensorRT_nvonnxparser_LIBRARY
    NAMES nvonnxparser
    HINTS
        ${TENSORRT_ROOT}
        $ENV{TENSORRT_ROOT}
        /usr/lib/x86_64-linux-gnu
        /usr/local/cuda/lib64
    PATH_SUFFIXES lib lib64
)

find_library(TensorRT_nvinfer_plugin_LIBRARY
    NAMES nvinfer_plugin
    HINTS
        ${TENSORRT_ROOT}
        $ENV{TENSORRT_ROOT}
        /usr/lib/x86_64-linux-gnu
        /usr/local/cuda/lib64
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TensorRT
    REQUIRED_VARS
        TensorRT_INCLUDE_DIR
        TensorRT_nvinfer_LIBRARY
        TensorRT_nvonnxparser_LIBRARY
)

if(TensorRT_FOUND)
    set(TensorRT_INCLUDE_DIRS ${TensorRT_INCLUDE_DIR})
    set(TensorRT_LIBRARIES
        ${TensorRT_nvinfer_LIBRARY}
        ${TensorRT_nvonnxparser_LIBRARY}
        ${TensorRT_nvinfer_plugin_LIBRARY}
    )

    # 尝试检测版本
    if(EXISTS "${TensorRT_INCLUDE_DIR}/NvInferVersion.h")
        file(READ "${TensorRT_INCLUDE_DIR}/NvInferVersion.h" _tensorrt_version_header)
        string(REGEX MATCH "define NV_TENSORRT_MAJOR ([0-9]+)" _match "${_tensorrt_version_header}")
        if(_match)
            set(TensorRT_VERSION_MAJOR "${CMAKE_MATCH_1}")
        endif()
        string(REGEX MATCH "define NV_TENSORRT_MINOR ([0-9]+)" _match "${_tensorrt_version_header}")
        if(_match)
            set(TensorRT_VERSION_MINOR "${CMAKE_MATCH_1}")
        endif()
        string(REGEX MATCH "define NV_TENSORRT_PATCH ([0-9]+)" _match "${_tensorrt_version_header}")
        if(_match)
            set(TensorRT_VERSION_PATCH "${CMAKE_MATCH_1}")
        endif()
        set(TensorRT_VERSION "${TensorRT_VERSION_MAJOR}.${TensorRT_VERSION_MINOR}.${TensorRT_VERSION_PATCH}")
    endif()

    mark_as_advanced(
        TensorRT_INCLUDE_DIR
        TensorRT_nvinfer_LIBRARY
        TensorRT_nvonnxparser_LIBRARY
        TensorRT_nvinfer_plugin_LIBRARY
    )
endif()
