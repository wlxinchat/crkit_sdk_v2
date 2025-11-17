cmake_minimum_required(VERSION 3.15)

# 简单的API测试
add_executable(test_api test_api.cpp)
target_link_libraries(test_api crkit ${OpenCV_LIBS})

# 后端测试
add_executable(test_backend test_backend.cpp)
target_link_libraries(test_backend crkit ${OpenCV_LIBS})

# 添加测试
add_test(NAME APITest COMMAND test_api)
add_test(NAME BackendTest COMMAND test_backend)

# 安装测试程序
install(TARGETS test_api test_backend
    RUNTIME DESTINATION bin/tests
)
