# =============================================================================
# Server Biz子模块 - 业务逻辑层
# =============================================================================

set(SERVER_BIZ_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/service.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/statistics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/statistics_service.h
)

set(SERVER_BIZ_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/biz/service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/biz/statistics.cpp
)

add_library(SmartSchedServerBiz STATIC
    ${SERVER_BIZ_HEADERS}
    ${SERVER_BIZ_SOURCES}
)

target_include_directories(SmartSchedServerBiz PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

target_link_libraries(SmartSchedServerBiz PUBLIC
    SmartSchedCommon
    SmartSchedServerCore
    SmartSchedServerDB
    Qt6::Core
)

# 安装
install(TARGETS SmartSchedServerBiz
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)
