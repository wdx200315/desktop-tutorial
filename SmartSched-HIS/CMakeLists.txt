# =============================================================================
# SmartSched-HIS: 医院门诊智慧调度系统 - 顶层CMake配置
# =============================================================================
# 医院视角: 企业级C/S架构，高并发、高安全、易部署
# 患者视角: 快速响应、清晰指引、减少等待
# 医生视角: 高效接诊、智能辅助、数据洞察
# 开发者视角: 跨平台、模块化、易维护
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# -----------------------------------------------------------------------------
# 项目信息
# -----------------------------------------------------------------------------
project(SmartSched-HIS
    VERSION 1.0.0
    DESCRIPTION "智序医院门诊智慧调度系统 - Smart Hospital Scheduling System"
    HOMEPAGE_URL "https://github.com/smartsched/his"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# C++标准配置
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++编译器警告优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-deprecated-copy
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /WX-)
endif()

# -----------------------------------------------------------------------------
# 输出配置
# -----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Debug/Release配置
set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_RELEASE_POSTFIX "")

# -----------------------------------------------------------------------------
# 平台检测与差异化配置
# -----------------------------------------------------------------------------
if(WIN32)
    set(PLATFORM_NAME "Windows")
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
elseif(UNIX AND NOT APPLE)
    if(EXISTS "/etc/os-release")
        file(STRINGS "/etc/os-release" OS_ID REGEX "^ID=")
        string(REGEX REPLACE "^ID=" "" OS_ID "${OS_ID}")
        string(TRIM "\"" OS_ID)
    endif()
    if(OS_ID STREQUAL "linuxmint" OR EXISTS "/etc/kylin-release")
        set(PLATFORM_NAME "Kylin")
        set(IS_CHINESE_OS ON)
    else()
        set(PLATFORM_NAME "Linux")
    endif()
    add_compile_definitions(PLATFORM_LINUX=1)
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
    add_compile_definitions(PLATFORM_MACOS=1)
endif()

message(STATUS "=================================================")
message(STATUS "SmartSched-HIS 配置信息")
message(STATUS "=================================================")
message(STATUS "平台: ${PLATFORM_NAME}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: C++${CMAKE_CXX_STANDARD}")
message(STATUS "项目目录: ${CMAKE_SOURCE_DIR}")
message(STATUS "=================================================")

# -----------------------------------------------------------------------------
# Qt6配置（客户端必需）
# -----------------------------------------------------------------------------
# 设置Qt6搜索路径（支持自定义安装路径）
set(QT_MIN_VERSION 6.11.1)
set(QT_VERSION_MAJOR 6)

# 优先使用环境变量中的Qt路径
if(DEFINED ENV{QT_PATH})
    list(PREPEND CMAKE_PREFIX_PATH $ENV{QT_PATH})
endif()

# 查找Qt6（使用Config模式，更现代）
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core        # 核心功能
    Network     # 网络通信（TCP）
    Widgets     # Widgets界面（医生工作站、管理端）
    Qml         # QML引擎
    Quick       # Quick控件（患者终端、排班看板）
    QuickControls2  # Qt Quick Controls 2
)

# 显示Qt6查找结果
get_target_property(QT_CORE_TYPE Qt6::Core TYPE)
message(STATUS "Qt6 Core类型: ${QT_CORE_TYPE}")
message(STATUS "Qt6版本: ${Qt6_VERSION}")

# 启用Qt自动MOC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6的Objective-C++支持（macOS）
if(APPLE)
    enable_language(OBJCXX)
endif()

# =============================================================================
# 依赖库查找
# =============================================================================

# OpenSSL配置 - Windows Qt环境特殊处理
if(WIN32)
    # 首先尝试使用CMake的标准查找
    find_package(OpenSSL QUIET)
    
    if(NOT OpenSSL_FOUND)
        message(STATUS "OpenSSL not found with standard search, trying custom paths...")
        
        # MinGW常用的OpenSSL路径
        set(MINGW_OPENSSL_SEARCH_PATHS 
            "C:/OpenSSL-Win64"
            "C:/OpenSSL-Win32"
            "A:/OpenSSL-Win64"
            "A:/OpenSSL-Win32"
            "C:/Program Files/OpenSSL-Win64"
            "C:/Program Files (x86)/OpenSSL-Win32"
            "${CMAKE_PREFIX_PATH}/lib/OpenSSL-Win64"
            "${QT_PREFIX_PATH}/../OpenSSL-Win64"
        )
        
        foreach(search_path IN LISTS MINGW_OPENSSL_SEARCH_PATHS)
            if(EXISTS "${search_path}")
                set(OPENSSL_ROOT_DIR "${search_path}" CACHE PATH "OpenSSL root directory" FORCE)
                message(STATUS "Trying OpenSSL at: ${search_path}")
                
                # 设置lib和include路径
                if(EXISTS "${search_path}/lib")
                    list(APPEND OPENSSL_LIBRARY_DIRS "${search_path}/lib")
                endif()
                if(EXISTS "${search_path}/include")
                    list(APPEND OPENSSL_INCLUDE_DIRS "${search_path}/include")
                endif()
                
                find_package(OpenSSL QUIET)
                if(OpenSSL_FOUND)
                    message(STATUS "Found OpenSSL at: ${search_path}")
                    break()
                endif()
            endif()
        endforeach()
    endif()
    
    # 如果仍然找不到，使用备用方案（禁用加密或使用Qt的SSL）
    if(NOT OpenSSL_FOUND)
        message(WARNING "========================================================")
        message(WARNING "OpenSSL not found! Using fallback configuration.")
        message(WARNING "加密功能可能不可用，但项目仍可构建。")
        message(WARNING "如需加密功能，请安装 OpenSSL: https://slproweb.com/products/Win32OpenSSL.html")
        message(WARNING "========================================================")
        
        # 创建模拟的OpenSSL配置（用于不需要加密的场景）
        set(OPENSSL_FOUND TRUE)
        set(OPENSSL_VERSION "1.0.0 (fallback)")
        set(OPENSSL_INCLUDE_DIR "")
        set(OPENSSL_CRYPTO_LIBRARY "crypt32")
        set(OPENSSL_SSL_LIBRARY "sslcapi")
        
        # 定义一个标志表示使用备用方案
        add_compile_definitions(OPENSSL_FALLBACK=1)
    endif()
else()
    # Linux/macOS使用标准查找
    find_package(OpenSSL REQUIRED)
endif()

message(STATUS "OpenSSL版本: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# MySQL配置（数据持久化）
# -----------------------------------------------------------------------------
# 尝试多种MySQL查找方式（兼容不同平台）

# 方式1: CMake的FindMySQL（Unix-like）
find_package(MySQL)

# 方式2: pkg-config（Linux）
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(MYSQL_CLIENT QUIET mysqlclient)
endif()

# 方式3: Windows环境变量
if(WIN32)
    if(DEFINED ENV{MYSQL_ROOT_DIR})
        set(MYSQL_ROOT_DIR $ENV{MYSQL_ROOT_DIR})
        set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT_DIR}/include")
        set(MYSQL_LIBRARY "${MYSQL_ROOT_DIR}/lib/libmysql.lib")
    endif()
endif()

# 显示MySQL查找结果
if(MySQL_FOUND)
    message(STATUS "MySQL: Found (using CMake FindMySQL)")
elseif(MYSQL_CLIENT_FOUND)
    message(STATUS "MySQL: Found (using pkg-config)")
    set(MYSQL_INCLUDE_DIR ${MYSQL_CLIENT_INCLUDE_DIRS})
    set(MYSQL_LIBRARIES ${MYSQL_CLIENT_LIBRARIES})
elseif(DEFINED MYSQL_INCLUDE_DIR)
    message(STATUS "MySQL: Found (using manual path)")
else()
    message(WARNING "MySQL: Not found, 数据库模块将被禁用")
endif()

# -----------------------------------------------------------------------------
# 选项配置
# -----------------------------------------------------------------------------
option(BUILD_SERVER "构建服务端" ON)
option(BUILD_CLIENT_PATIENT "构建患者挂号终端" ON)
option(BUILD_CLIENT_DOCTOR "构建医生工作站" ON)
option(BUILD_CLIENT_DISPLAY "构建排队看板" ON)
option(BUILD_CLIENT_ADMIN "构建管理端" ON)
option(BUILD_STRESSTEST "构建压力测试工具" ON)
option(BUILD_TESTS "构建单元测试" ON)
option(ENABLE_CRYPTO "启用AES-256加密" ON)
option(ENABLE_LOGGING "启用日志系统" ON)
option(ENABLE_WERROR "将警告视为错误" OFF)
option(ENABLE_DOCKER "启用Docker支持" ON)

# -----------------------------------------------------------------------------
# 子项目添加
# -----------------------------------------------------------------------------
# 公共模块（所有项目依赖）
add_subdirectory(common)

# 服务端模块
if(BUILD_SERVER)
    add_subdirectory(server)
endif()

# 客户端模块（患者终端、医生工作站、排班看板、管理端）
if(BUILD_CLIENT_PATIENT OR BUILD_CLIENT_DOCTOR OR BUILD_CLIENT_DISPLAY OR BUILD_CLIENT_ADMIN)
    add_subdirectory(client)
endif()

# 压力测试工具
if(BUILD_STRESSTEST)
    add_subdirectory(tools/stresstest)
endif()

# 测试模块
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# -----------------------------------------------------------------------------
# 导出配置（供外部项目使用）
# -----------------------------------------------------------------------------
export(TARGETS SmartSchedCommon
    FILE "${CMAKE_BINARY_DIR}/SmartSchedTargets.cmake"
)

# 生成包配置文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/SmartSchedConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/SmartSchedConfig.cmake"
    @ONLY
)

# -----------------------------------------------------------------------------
# 安装目标
# -----------------------------------------------------------------------------
install(DIRECTORY doc/
    DESTINATION share/doc/${PROJECT_NAME}
    COMPONENT documentation
)

# -----------------------------------------------------------------------------
# 自定义目标
# -----------------------------------------------------------------------------

# 清理构建产物
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build . --target clean
    COMMAND rm -rf ${CMAKE_BINARY_DIR}
    COMMENT "清理所有构建产物"
)

# 显示依赖树
add_custom_target(deps
    COMMAND ${CMAKE_COMMAND} --build . --target help | findstr /C:"all" /C:"SmartSched"
    COMMENT "显示项目依赖关系"
)

# 重新配置
add_custom_target(reconfigure
    COMMAND rm -rf ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -S${CMAKE_SOURCE_DIR} -B${CMAKE_BINARY_DIR}
    COMMENT "重新配置项目"
)
