# =============================================================================
# Admin Dashboard - 管理端
# =============================================================================

set(ADMIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/admin/adminmain.cpp
)

add_executable(SmartSchedAdmin ${ADMIN_SOURCES})

target_include_directories(SmartSchedAdmin PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SmartSchedAdmin PRIVATE
    SmartSchedCommon
    SmartSchedClientBase
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# 安装
install(TARGETS SmartSchedAdmin
    RUNTIME DESTINATION bin
    COMPONENT admin
)
