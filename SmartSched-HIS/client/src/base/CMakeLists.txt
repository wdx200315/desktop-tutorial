# =============================================================================
# Client Base - 客户端基础库
# 包含网络通信、数据模型等公共功能
# =============================================================================

set(CLIENT_BASE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/client/networkclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/client/datamodel.h
)

set(CLIENT_BASE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/networkclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/datamodel.cpp
)

add_library(SmartSchedClientBase STATIC
    ${CLIENT_BASE_HEADERS}
    ${CLIENT_BASE_SOURCES}
)

target_include_directories(SmartSchedClientBase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

target_link_libraries(SmartSchedClientBase PUBLIC
    SmartSchedCommon
    Qt6::Core
    Qt6::Network
)

# 安装
install(TARGETS SmartSchedClientBase
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)
