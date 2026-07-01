/**
 * @file macros.h
 * @brief 通用宏定义
 * 
 * 开发者视角: 提供便捷的开发工具宏
 */

#pragma once

#include "compiler.h"

// =============================================================================
// 内存管理宏（防止内存泄漏）
// =============================================================================
#define SMARTSCHED_DELETE_PTR(ptr) \
    do { delete (ptr); (ptr) = nullptr; } while(0)

#define SMARTSCHED_DELETE_ARRAY(ptr) \
    do { delete[] (ptr); (ptr) = nullptr; } while(0)

// =============================================================================
// 字符串工具宏
// =============================================================================
#define SMARTSCHED_STRINGIFY(x) #x
#define SMARTSCHED_STRINGIFY_IMPL(x) #x

#define SMARTSCHED_CONCAT(a, b) a##b
#define SMARTSCHED_CONCAT_IMPL(a, b) a##b

// =============================================================================
// 数组大小宏
// =============================================================================
#define SMARTSCHED_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// =============================================================================
// 禁用拷贝/移动构造
// =============================================================================
#define SMARTSCHED_DISALLOW_COPY(ClassName) \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

#define SMARTSCHED_DISALLOW_MOVE(ClassName) \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

#define SMARTSCHED_DISALLOW_COPY_AND_MOVE(ClassName) \
    SMARTSCHED_DISALLOW_COPY(ClassName) \
    SMARTSCHED_DISALLOW_MOVE(ClassName)

// =============================================================================
// RAII作用域锁
// =============================================================================
#ifdef SMARTSCHED_PLATFORM_WINDOWS
    #include <windows.h>
    
    #define SMARTSCHED_SCOPED_LOCK(mutex) \
        std::unique_lock<std::mutex> SMARTSCHED_UNIQUE_NAME(_lock)(mutex)
#else
    #include <mutex>
    #define SMARTSCHED_SCOPED_LOCK(mutex) \
        std::lock_guard<std::mutex> SMARTSCHED_UNIQUE_NAME(_lock)(mutex)
#endif

// =============================================================================
// 唯一变量名生成
// =============================================================================
#define SMARTSCHED_UNIQUE_NAME(prefix) SMARTSCHED_CONCAT(prefix, __LINE__)

// =============================================================================
// 编译时断言
// =============================================================================
#define SMARTSCHED_STATIC_ASSERT(expr, msg) \
    static_assert(expr, msg)

// =============================================================================
// 日志级别宏
// =============================================================================
namespace smartsched {
namespace common {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

} // namespace common
} // namespace smartsched

// =============================================================================
// 调试辅助
// =============================================================================
#ifdef NDEBUG
    #define SMARTSCHED_ASSERT(expr, msg) ((void)0)
    #define SMARTSCHED_DEBUG_ONLY(code) ((void)0)
#else
    #include <cassert>
    #define SMARTSCHED_ASSERT(expr, msg) assert((expr) && (msg))
    #define SMARTSCHED_DEBUG_ONLY(code) code
#endif

// =============================================================================
// 属性标签（用于代码文档化）
// =============================================================================
#define SMARTSCHED_NODISCARD [[nodiscard]]
#define SMARTSCHED_OVERRIDE override
#define SMARTSCHED_FINAL final
#define SMARTSCHED_NOEXCEPT noexcept

// =============================================================================
// 可选参数支持（C++17前兼容）
// =============================================================================
#if __cplusplus < 201703L
    #include <optional>
    namespace std {
        using std::optional;
        using std::nullopt;
        using std::make_optional;
    }
#else
    #include <optional>
#endif

// =============================================================================
// 字符串字面量
// =============================================================================
namespace smartsched {
namespace literals {

constexpr size_t operator"" _z(unsigned long long n) { return static_cast<size_t>(n); }

} // namespace literals
} // namespace smartsched

using namespace smartsched::literals;
