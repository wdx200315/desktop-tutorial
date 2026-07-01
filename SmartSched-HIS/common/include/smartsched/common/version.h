/**
 * @file version.h
 * @brief 版本信息定义
 * 
 * 医院视角: 版本追踪用于合规审计
 * 运维视角: 快速识别部署版本
 */

#pragma once

#include "compiler.h"

// =============================================================================
// 版本号定义（语义化版本 Semantic Versioning）
// =============================================================================
// 主版本号：不兼容的API变更
#define SMARTSCHED_VERSION_MAJOR 1
// 次版本号：向后兼容的功能新增
#define SMARTSCHED_VERSION_MINOR 0
// 修订号：向后兼容的问题修复
#define SMARTSCHED_VERSION_PATCH 0
// 构建元数据（如Beta、RC等）
#define SMARTSCHED_VERSION_BUILD "alpha"

// =============================================================================
// 版本字符串
// =============================================================================
#define SMARTSCHED_VERSION_STRING \
    SMARTSCHED_TO_STRING(SMARTSCHED_VERSION_MAJOR) "." \
    SMARTSCHED_TO_STRING(SMARTSCHED_VERSION_MINOR) "." \
    SMARTSCHED_TO_STRING(SMARTSCHED_VERSION_PATCH) \
    "-" SMARTSCHED_VERSION_BUILD

#define SMARTSCHED_FULL_NAME "智序医院门诊智慧调度系统"
#define SMARTSCHED_SHORT_NAME "SmartSched-HIS"
#define SMARTSCHED_COMPANY "SmartSched Healthcare Technology"
#define SMARTSCHED_COPYRIGHT "Copyright (C) 2024-2026"

// =============================================================================
// 内部使用
// =============================================================================
#define SMARTSCHED_TO_STRING_IMPL(x) #x
#define SMARTSCHED_TO_STRING(x) SMARTSCHED_TO_STRING_IMPL(x)

// =============================================================================
// 版本检查宏
// =============================================================================
#define SMARTSCHED_VERSION_CHECK(major, minor, patch) \
    (SMARTSCHED_VERSION_MAJOR > (major) || \
     (SMARTSCHED_VERSION_MAJOR == (major) && SMARTSCHED_VERSION_MINOR > (minor)) || \
     (SMARTSCHED_VERSION_MAJOR == (major) && SMARTSCHED_VERSION_MINOR == (minor) && SMARTSCHED_VERSION_PATCH >= (patch)))

// =============================================================================
// 版本信息结构
// =============================================================================
namespace smartsched {
namespace common {

struct Version {
    int major;
    int minor;
    int patch;
    const char* build;
    
    constexpr Version(int maj, int min, int pat, const char* bld = "")
        : major(maj), minor(min), patch(pat), build(bld) {}
    
    static constexpr Version current() {
        return Version(SMARTSCHED_VERSION_MAJOR, 
                      SMARTSCHED_VERSION_MINOR, 
                      SMARTSCHED_VERSION_PATCH,
                      SMARTSCHED_VERSION_BUILD);
    }
    
    bool isCompatibleWith(const Version& other) const {
        return major == other.major && 
               (minor > other.minor || (minor == other.minor && patch >= other.patch));
    }
};

} // namespace common
} // namespace smartsched
