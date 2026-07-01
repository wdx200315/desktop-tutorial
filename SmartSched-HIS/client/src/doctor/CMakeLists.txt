# =============================================================================
# Doctor Workstation - 医生工作站
# =============================================================================

set(DOCTOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/doctor/doctormain.cpp
)

add_executable(SmartSchedDoctor ${DOCTOR_SOURCES})

target_include_directories(SmartSchedDoctor PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SmartSchedDoctor PRIVATE
    SmartSchedCommon
    SmartSchedClientBase
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# 安装
install(TARGETS SmartSchedDoctor
    RUNTIME DESTINATION bin
    COMPONENT client
)
