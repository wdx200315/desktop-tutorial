cmake_minimum_required(VERSION 3.16)
project(ChargePulseMobile)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 查找路径
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/mingw81_64" CACHE PATH "Qt installation prefix")

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Network)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 源文件
set(SOURCES
    src/main.cpp
    src/NetworkManager.cpp
    src/ThemeManager.cpp
)

add_executable(ChargePulseMobile ${SOURCES})

target_link_libraries(ChargePulseMobile
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

# 添加 QML 资源
qt_add_resources(ChargePulseMobile "resources" FILES resources.qrc)

message(STATUS "========================================")
message(STATUS "ChargePulseMobile Build")
message(STATUS "Qt6 found: ${Qt6_FOUND}")
message(STATUS "========================================")
