cmake_minimum_required(VERSION 3.15)

# 简单推理示例
add_executable(simple_inference simple_inference.c)
target_link_libraries(simple_inference crkit ${OpenCV_LIBS})

# 批量推理示例
add_executable(batch_inference batch_inference.c)
target_link_libraries(batch_inference crkit ${OpenCV_LIBS})

# 安装示例
install(TARGETS simple_inference batch_inference
    RUNTIME DESTINATION bin/examples
)

# 拷贝示例图像
if(EXISTS "${CMAKE_SOURCE_DIR}/examples/test.jpg")
    install(FILES test.jpg DESTINATION bin/examples)
endif()
