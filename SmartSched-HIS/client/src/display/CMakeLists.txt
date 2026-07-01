# =============================================================================
# Display Board - 排队叫号大屏
# =============================================================================

set(DISPLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/display/displaymain.cpp
)

add_executable(SmartSchedDisplay ${DISPLAY_SOURCES})

target_include_directories(SmartSchedDisplay PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SmartSchedDisplay PRIVATE
    SmartSchedCommon
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
)

# Qt资源
target_sources(SmartSchedDisplay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/display/DisplayBoard.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/DisplayBoard.qrc
)

# 安装
install(TARGETS SmartSchedDisplay
    RUNTIME DESTINATION bin
    COMPONENT client
)
