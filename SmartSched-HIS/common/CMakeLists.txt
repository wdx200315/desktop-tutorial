# =============================================================================
# Common模块 - 公共协议、常量、工具类
# 所有服务端和客户端共享的基础组件
# =============================================================================

# 源码文件列表
set(COMMON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/common/compiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/common/version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/common/macros.h
)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.cpp
)

# 协议定义
set(PROTOCOL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/protocol/protocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/protocol/commands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/protocol/message.h
)

# 工具类
set(UTILS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/bytebuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/datetime.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/json_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/filelogger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/utils/csv_exporter.h
)

set(UTILS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/config.cpp
)

# 创建公共库
add_library(SmartSchedCommon STATIC
    ${COMMON_HEADERS}
    ${COMMON_SOURCES}
    ${PROTOCOL_HEADERS}
    ${UTILS_HEADERS}
    ${UTILS_SOURCES}
)

# 公共库属性
target_include_directories(SmartSchedCommon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(SmartSchedCommon PRIVATE
    SMART_SCHED_VERSION=${PROJECT_VERSION}
)

# 链接OpenSSL（用于加密工具函数）
if(OPENSSL_FALLBACK)
    # 备用方案：使用系统自带的加密库
    target_link_libraries(SmartSchedCommon PUBLIC
        crypt32
    )
    message(STATUS "Using fallback crypto library (no OpenSSL)")
else()
    target_link_libraries(SmartSchedCommon PUBLIC
        OpenSSL::Crypto
    )
endif()

# Windows特定配置
if(WIN32)
    target_compile_definitions(SmartSchedCommon PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
    
    # 使用静态CRT避免DLL依赖问题
    if(MSVC)
        target_compile_options(SmartSchedCommon PRIVATE
            /MT$<$<CONFIG:Debug>:d>
        )
    endif()
endif()

# 安装配置
install(TARGETS SmartSchedCommon
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)

install(DIRECTORY include/smartsched
    DESTINATION include
    COMPONENT headers
)
