# =============================================================================
# Patient Terminal - 患者挂号终端
# =============================================================================

set(PATIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/patient/patientmain.cpp
)

add_executable(SmartSchedPatient ${PATIENT_SOURCES})

target_include_directories(SmartSchedPatient PRIVATE
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SmartSchedPatient PRIVATE
    SmartSchedCommon
    SmartSchedClientBase
    Qt6::Core
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# Qt资源
target_sources(SmartSchedPatient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/patient/main.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/patient/DepartmentList.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/patient/RegistrationPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/patient/QueueStatusPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/patient/SuccessDialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/PatientTerminal.qrc
)

# 安装
install(TARGETS SmartSchedPatient
    RUNTIME DESTINATION bin
    COMPONENT client
)
