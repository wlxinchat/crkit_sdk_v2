# FindONNXRuntime.cmake
# 查找ONNX Runtime库

find_path(ONNXRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS
        ${ONNXRUNTIME_ROOT}
        $ENV{ONNXRUNTIME_ROOT}
        /usr/include
        /usr/local/include
    PATH_SUFFIXES include onnxruntime/core/session
)

find_library(ONNXRuntime_LIBRARY
    NAMES onnxruntime
    HINTS
        ${ONNXRUNTIME_ROOT}
        $ENV{ONNXRUNTIME_ROOT}
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime
    REQUIRED_VARS
        ONNXRuntime_INCLUDE_DIR
        ONNXRuntime_LIBRARY
)

if(ONNXRuntime_FOUND)
    set(ONNXRuntime_INCLUDE_DIRS ${ONNXRuntime_INCLUDE_DIR})
    set(ONNXRuntime_LIBRARIES ${ONNXRuntime_LIBRARY})

    mark_as_advanced(
        ONNXRuntime_INCLUDE_DIR
        ONNXRuntime_LIBRARY
    )
endif()
