# =============================================================================
# Server DB子模块 - MySQL数据库连接池
# =============================================================================

set(SERVER_DB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/smartsched/server/database.h
)

set(SERVER_DB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/db/database.cpp
)

add_library(SmartSchedServerDB STATIC
    ${SERVER_DB_HEADERS}
    ${SERVER_DB_SOURCES}
)

target_include_directories(SmartSchedServerDB PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

# MySQL条件链接
if(MySQL_FOUND)
    target_include_directories(SmartSchedServerDB PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(SmartSchedServerDB PRIVATE ${MYSQL_LIBRARIES})
elseif(MYSQL_CLIENT_FOUND)
    target_include_directories(SmartSchedServerDB PRIVATE ${MYSQL_CLIENT_INCLUDE_DIRS})
    target_link_libraries(SmartSchedServerDB PRIVATE ${MYSQL_CLIENT_LIBRARIES})
elseif(DEFINED MYSQL_INCLUDE_DIR)
    target_include_directories(SmartSchedServerDB PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(SmartSchedServerDB PRIVATE ${MYSQL_LIBRARY})
endif()

target_link_libraries(SmartSchedServerDB PUBLIC
    SmartSchedCommon
    Qt6::Core
)

# 安装
install(TARGETS SmartSchedServerDB
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT libraries
)
