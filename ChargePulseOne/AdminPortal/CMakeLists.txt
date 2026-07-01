cmake_minimum_required(VERSION 3.16)
project(AdminPortal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    src/main.cpp
    src/AdminMainWindow.cpp
    src/LoginDialog.cpp
    src/NetworkManager.cpp
    src/DashboardWidget.cpp
    src/UserManagementWidget.cpp
    src/ChargerManagementWidget.cpp
    src/RateManagementWidget.cpp
    src/OrderManagementWidget.cpp
    src/AlarmManagementWidget.cpp
    src/ReportWidget.cpp
    src/SystemSettingsWidget.cpp
    src/LogAuditWidget.cpp
    src/ReservationManagementWidget.cpp
    src/CouponManagementWidget.cpp
    src/MemberManagementWidget.cpp
    src/HealthDashboardWidget.cpp
    src/FinanceReportWidget.cpp
    src/PushNotificationWidget.cpp
)
add_executable(AdminPortal ${SOURCES})
target_link_libraries(AdminPortal Qt6::Core Qt6::Widgets Qt6::Network)
qt_add_resources(AdminPortal "resources"
    PREFIX "/resources"
    FILES resources/styles/admin_style.qss
)
