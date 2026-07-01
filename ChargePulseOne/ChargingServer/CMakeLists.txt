cmake_minimum_required(VERSION 3.16)
project(ChargePulseOne CXX)

# ========== 编译器设置 ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用详细警告
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 调试/发布模式
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_MODE)
    message(STATUS "Build type: Debug")
else()
    add_compile_definitions(NDEBUG)
    message(STATUS "Build type: Release")
endif()

# ========== 依赖包 ==========
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# MySQL配置
if(WIN32)
    set(MYSQL_INCLUDE_DIR "C:/Program Files/MySQL/MySQL Server 8.0/include" CACHE PATH "MySQL Include Directory")
    set(MYSQL_LIB_DIR "C:/Program Files/MySQL/MySQL Server 8.0/lib" CACHE PATH "MySQL Library Directory")
    find_library(MYSQL_LIB libmysql.lib PATHS ${MYSQL_LIB_DIR} REQUIRED)
else()
    find_path(MYSQL_INCLUDE_DIR mysql/mysql.h REQUIRED)
    find_library(MYSQL_LIB mysqlclient REQUIRED)
endif()

include_directories(${MYSQL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

# ========== 源文件列表 ==========
set(SOURCES
    src/main.cpp
    src/TcpServer.cpp
    src/ClientHandler.cpp
    src/ProtocolParser.cpp
    src/DatabaseManager.cpp
    src/ConnectionPool.cpp
    src/ConfigManager.cpp
    src/Logger.cpp
    src/CryptoUtils.cpp
    src/SessionManager.cpp
    src/UserService.cpp
    src/ChargerService.cpp
    src/ChargingService.cpp
    src/BillingService.cpp
    src/PaymentService.cpp
    src/OrderService.cpp
    src/AlarmService.cpp
    src/ReportService.cpp
    src/ConfigService.cpp
    src/LogService.cpp
    src/ReservationService.cpp
    src/NotificationService.cpp
    src/CouponService.cpp
    src/MemberService.cpp
    src/AnalyticsService.cpp
    src/ModbusController.cpp
    src/MessageService.cpp
)

# ========== 头文件列表（用于IDE显示） ==========
set(HEADERS
    src/common.h
    src/TcpServer.h
    src/ClientHandler.h
    src/ProtocolParser.h
    src/DatabaseManager.h
    src/ConnectionPool.h
    src/ConfigManager.h
    src/Logger.h
    src/CryptoUtils.h
    src/SessionManager.h
    src/UserService.h
    src/ChargerService.h
    src/ChargingService.h
    src/BillingService.h
    src/PaymentService.h
    src/OrderService.h
    src/AlarmService.h
    src/ReportService.h
    src/ConfigService.h
    src/LogService.h
    src/ReservationService.h
    src/NotificationService.h
    src/CouponService.h
    src/MemberService.h
    src/AnalyticsService.h
    src/ModbusController.h
    src/MessageService.h
)

# ========== 生成可执行文件 ==========
add_executable(charge_server ${SOURCES} ${HEADERS})

target_link_libraries(charge_server
    ${MYSQL_LIB}
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
)

# Windows特定库
if(WIN32)
    target_link_libraries(charge_server ws2_32)
endif()

# ========== 安装规则 ==========
install(TARGETS charge_server DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/chargepulse)

# ========== 编译信息输出 ==========
message(STATUS "========================================")
message(STATUS "ChargePulse One Server Build Configuration")
message(STATUS "========================================")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "MySQL Include: ${MYSQL_INCLUDE_DIR}")
message(STATUS "MySQL Library: ${MYSQL_LIB}")
message(STATUS "========================================")
