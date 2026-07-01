cmake_minimum_required(VERSION 3.16)
project(smartsched-stresstest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含主项目
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_BINARY_DIR}/smartsched)

# 压力测试工具
add_executable(stresstest 
    src/stresstest.cpp
)

target_include_directories(stresstest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/server/include
)

target_link_libraries(stresstest
    smartsched-common
    Qt6::Network
)

# WS2_32 for Windows
if(WIN32)
    target_link_libraries(stresstest ws2_32)
endif()

# 安装
install(TARGETS stresstest DESTINATION bin)
