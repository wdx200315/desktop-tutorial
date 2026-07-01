/**
 * @file compiler.h
 * @brief 编译器兼容性检测与配置
 * 
 * 医院视角: 确保跨平台编译稳定性
 * 开发者视角: 标准化编译环境检测
 */

#pragma once

// =============================================================================
// 编译器检测
// =============================================================================
#ifdef __clang__
    #define SMARTSCHED_COMPILER_CLANG 1
    #define SMARTSCHED_COMPILER_NAME "Clang"
#elif defined(__GNUC__)
    #define SMARTSCHED_COMPILER_GCC 1
    #define SMARTSCHED_COMPILER_NAME "GCC"
    #define SMARTSCHED_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
    #define SMARTSCHED_COMPILER_MSVC 1
    #define SMARTSCHED_COMPILER_NAME "MSVC"
    #define SMARTSCHED_COMPILER_VERSION _MSC_VER
#else
    #define SMARTSCHED_COMPILER_NAME "Unknown"
#endif

// =============================================================================
// C++标准检测
// =============================================================================
#if __cplusplus >= 202302L
    #define SMARTSCHED_CXX_STANDARD 23
    #define SMARTSCHED_CXX_STANDARD_NAME "C++23"
#elif __cplusplus >= 202002L
    #define SMARTSCHED_CXX_STANDARD 20
    #define SMARTSCHED_CXX_STANDARD_NAME "C++20"
#elif __cplusplus >= 201703L
    #define SMARTSCHED_CXX_STANDARD 17
    #define SMARTSCHED_CXX_STANDARD_NAME "C++17"
#else
    #define SMARTSCHED_CXX_STANDARD 14
    #define SMARTSCHED_CXX_STANDARD_NAME "C++14"
#endif

// =============================================================================
// 平台检测
// =============================================================================
#ifdef _WIN32
    #define SMARTSCHED_PLATFORM_WINDOWS 1
    #define SMARTSCHED_PLATFORM_NAME "Windows"
    
    #ifdef _WIN64
        #define SMARTSCHED_ARCH_64BIT 1
    #else
        #define SMARTSCHED_ARCH_32BIT 1
    #endif
    
    #define SMARTSCHED_PATH_SEPARATOR '\\'
    #define SMARTSCHED_LINE_ENDING "\r\n"
    
#elif defined(__linux__)
    #define SMARTSCHED_PLATFORM_LINUX 1
    #define SMARTSCHED_PLATFORM_NAME "Linux"
    
    #if __x86_64__ || __ppc64__
        #define SMARTSCHED_ARCH_64BIT 1
    #else
        #define SMARTSCHED_ARCH_32BIT 1
    #endif
    
    #define SMARTSCHED_PATH_SEPARATOR '/'
    #define SMARTSCHED_LINE_ENDING "\n"
    
#elif defined(__APPLE__)
    #define SMARTSCHED_PLATFORM_MACOS 1
    #define SMARTSCHED_PLATFORM_NAME "macOS"
    #define SMARTSCHED_ARCH_64BIT 1
    
    #define SMARTSCHED_PATH_SEPARATOR '/'
    #define SMARTSCHED_LINE_ENDING "\n"
#else
    #define SMARTSCHED_PLATFORM_UNKNOWN 1
    #define SMARTSCHED_PLATFORM_NAME "Unknown"
#endif

// =============================================================================
// 废弃API警告
// =============================================================================
#if defined(SMARTSCHED_COMPILER_GCC) || defined(SMARTSCHED_COMPILER_CLANG)
    #define SMARTSCHED_DEPRECATED __attribute__((deprecated))
    #define SMARTSCHED_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#elif defined(SMARTSCHED_COMPILER_MSVC)
    #define SMARTSCHED_DEPRECATED __declspec(deprecated)
    #define SMARTSCHED_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#else
    #define SMARTSCHED_DEPRECATED
    #define SMARTSCHED_DEPRECATED_MSG(msg)
#endif

// =============================================================================
// 导出/导入宏（Windows DLL支持）
// =============================================================================
#ifdef SMARTSCHED_EXPORTS
    #define SMARTSCHED_API __declspec(dllexport)
#else
    #define SMARTSCHED_API __declspec(dllimport)
#endif

// =============================================================================
// 头文件防护
// =============================================================================
#define SMARTSCHED_BEGIN_HEADER_GUARD
#define SMARTSCHED_END_HEADER_GUARD
