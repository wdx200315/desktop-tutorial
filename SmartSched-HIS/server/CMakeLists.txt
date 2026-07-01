# =============================================================================
# Server模块 - 服务端核心
# 支持多线程TCP网络、AES加密、MySQL数据库
# =============================================================================

# Server子目录
add_subdirectory(src/core)      # 核心网络层
add_subdirectory(src/crypto)    # 加密模块
add_subdirectory(src/db)        # 数据库模块
add_subdirectory(src/biz)       # 业务逻辑模块

# 服务端可执行文件
set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

# 添加公共头文件路径
set(SERVER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

# 创建可执行文件
add_executable(SmartSchedServer ${SERVER_SOURCES})

# 链接目录
target_include_directories(SmartSchedServer PRIVATE
    ${SERVER_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(SmartSchedServer PRIVATE
    SmartSchedCommon
    SmartSchedServerCore
    SmartSchedServerCrypto
    SmartSchedServerDB
    SmartSchedServerBiz
    Qt6::Core
    Qt6::Network
)

# OpenSSL链接（条件）
if(OPENSSL_FALLBACK)
    # 备用方案
    message(STATUS "Server: Using fallback crypto")
else()
    target_link_libraries(SmartSchedServer PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

# MySQL库（条件链接）
if(MySQL_FOUND)
    target_include_directories(SmartSchedServer PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(SmartSchedServer PRIVATE ${MYSQL_LIBRARIES})
elseif(MYSQL_CLIENT_FOUND)
    target_include_directories(SmartSchedServer PRIVATE ${MYSQL_CLIENT_INCLUDE_DIRS})
    target_link_libraries(SmartSchedServer PRIVATE ${MYSQL_CLIENT_LIBRARIES})
elseif(DEFINED MYSQL_INCLUDE_DIR)
    target_include_directories(SmartSchedServer PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(SmartSchedServer PRIVATE ${MYSQL_LIBRARY})
endif()

# Windows特定配置
if(WIN32)
    target_link_libraries(SmartSchedServer PRIVATE ws2_32 wsock32)
endif()

# 安装
install(TARGETS SmartSchedServer
    RUNTIME DESTINATION bin
    COMPONENT server
)
