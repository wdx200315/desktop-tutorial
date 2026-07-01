# =============================================================================
# Server Core子模块 - 多线程TCP网络层
# =============================================================================

set(SERVER_CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/tcpserver.h
)

set(SERVER_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/tcpserver.cpp
)

add_library(SmartSchedServerCore STATIC
    ${SERVER_CORE_HEADERS}
    ${SERVER_CORE_SOURCES}
)

target_include_directories(SmartSchedServerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

target_link_libraries(SmartSchedServerCore PUBLIC
    SmartSchedCommon
    Qt6::Core
    Qt6::Network
)

# Windows特定
if(WIN32)
    target_link_libraries(SmartSchedServerCore PRIVATE ws2_32 wsock32)
endif()

# 安装
install(TARGETS SmartSchedServerCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)
