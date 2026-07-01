cmake_minimum_required(VERSION 3.16)

# =============================================================================
# SmartSched-HIS 测试模块
# =============================================================================

# GoogleTest配置
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# Windows特定设置
if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

# 启用测试
enable_testing()

# 测试可执行文件
set(TEST_SOURCES
    test_common.cpp
    test_protocol.cpp
    test_crypto.cpp
    test_database.cpp
    test_auth.cpp
    test_cache.cpp
    test_security.cpp
    test_integration.cpp
)

add_executable(smartsched-test ${TEST_SOURCES})

target_include_directories(smartsched-test PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/server/include
    ${CMAKE_SOURCE_DIR}/client/include
)

target_link_libraries(smartsched-test
    GTest::gtest_main
    GTest::gmock_main
    SmartSchedCommon
    Qt6::Core
    Qt6::Network
)

# 平台特定链接
if(WIN32)
    target_link_libraries(smartsched-test ws2_32)
elseif(UNIX)
    target_link_libraries(smartsched-test pthread)
endif()

# 注册测试
include(GoogleTest)
gtest_discover_tests(smartsched-test)

# 添加自定义测试目标
add_custom_target(run-tests
    COMMAND $<TARGET_FILE:smartsched-test>
    COMMENT "运行所有测试"
)

add_custom_target(test-coverage
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/coverage.sh
    COMMENT "生成代码覆盖率报告"
)
