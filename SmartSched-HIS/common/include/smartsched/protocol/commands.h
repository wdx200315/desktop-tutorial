/**
 * @file commands.h
 * @brief 业务命令定义
 * 
 * 患者视角: 简化的挂号流程命令
 * 医生视角: 高效的接诊操作命令
 * 管理员视角: 丰富的管理配置命令
 */

#pragma once

#include "protocol.h"
#include <string>

namespace smartsched {
namespace protocol {

// =============================================================================
// 命令ID枚举
// =============================================================================
enum class CommandID : uint16_t {
    // ==================== 患者端命令 ====================
    
    // 科室相关
    DEPT_LIST = 1001,          // 获取科室列表
    DEPT_INFO = 1002,           // 获取科室详情
    
    // 医生相关
    DOCTOR_LIST = 1011,        // 获取科室医生列表
    DOCTOR_INFO = 1012,        // 获取医生详情
    DOCTOR_SCHEDULE = 1013,    // 获取医生排班
    
    // 挂号相关
    REGISTER = 1021,           // 患者挂号
    CANCEL_REGISTER = 1022,    // 取消挂号
    GET_QUEUE_NUM = 1023,      // 获取排队号码
    
    // 排队相关
    QUEUE_STATUS = 1031,       // 查询排队状态
    ESTIMATED_WAIT = 1032,     // 预估等待时间
    
    // ==================== 医生端命令 ====================
    
    // 接诊相关
    CONSULT_START = 2001,      // 开始接诊
    CONSULT_END = 2002,        // 结束接诊
    CONSULT_PAUSE = 2003,      // 暂停接诊
    
    // 患者相关
    PATIENT_INFO = 2011,       // 获取患者信息
    PATIENT_HISTORY = 2012,    // 获取患者历史记录
    
    // B超相关
    ULTRASOUND_REQUEST = 2021, // 申请B超检查
    ULTRASOUND_CANCEL = 2022, // 取消B超申请
    
    // ==================== 叫号相关 ====================
    CALL_NEXT = 3001,          // 叫下一位
    CALL_SPECIFIC = 3002,      // 叫指定患者
    RECALL = 3003,             // 重叫
    SKIP = 3004,               // 跳过患者
    
    // ==================== B超室命令 ====================
    ULTRASOUND_STATUS = 4001,  // B超室状态
    ULTRASOUND_READY = 4002,   // B超室就绪
    ULTRASOUND_COMPLETE = 4003, // B超完成
    
    // ==================== 管理员命令 ====================
    
    // 数据统计
    STATISTICS = 5001,        // 统计数据
    STATISTICS_DAILY = 5002,   // 日报
    STATISTICS_MONTHLY = 5003,// 月报
    
    // 配置管理
    CONFIG_GET = 5101,        // 获取配置
    CONFIG_SET = 5102,        // 设置配置
    
    // 医生管理
    DOCTOR_ADD = 5111,         // 添加医生
    DOCTOR_UPDATE = 5112,      // 更新医生信息
    DOCTOR_DELETE = 5113,      // 删除医生
    
    // 报表导出
    REPORT_EXPORT = 5201,      // 导出报表
    
    // ==================== 系统命令 ====================
    PING = 9001,              // 心跳/ping
    PONG = 9002,              // 心跳响应/pong
    SERVER_INFO = 9011,       // 获取服务器信息
    VERSION_CHECK = 9012      // 版本检查
};

// =============================================================================
// 命令到字符串的映射（用于日志和调试）
// =============================================================================
SMARTSCHED_NODISCARD
inline const char* commandToString(CommandID cmd) {
    switch (cmd) {
        // 患者端
        case CommandID::DEPT_LIST: return "DEPT_LIST";
        case CommandID::DEPT_INFO: return "DEPT_INFO";
        case CommandID::DOCTOR_LIST: return "DOCTOR_LIST";
        case CommandID::DOCTOR_INFO: return "DOCTOR_INFO";
        case CommandID::DOCTOR_SCHEDULE: return "DOCTOR_SCHEDULE";
        case CommandID::REGISTER: return "REGISTER";
        case CommandID::CANCEL_REGISTER: return "CANCEL_REGISTER";
        case CommandID::GET_QUEUE_NUM: return "GET_QUEUE_NUM";
        case CommandID::QUEUE_STATUS: return "QUEUE_STATUS";
        case CommandID::ESTIMATED_WAIT: return "ESTIMATED_WAIT";
        
        // 医生端
        case CommandID::CONSULT_START: return "CONSULT_START";
        case CommandID::CONSULT_END: return "CONSULT_END";
        case CommandID::CONSULT_PAUSE: return "CONSULT_PAUSE";
        case CommandID::PATIENT_INFO: return "PATIENT_INFO";
        case CommandID::PATIENT_HISTORY: return "PATIENT_HISTORY";
        case CommandID::ULTRASOUND_REQUEST: return "ULTRASOUND_REQUEST";
        case CommandID::ULTRASOUND_CANCEL: return "ULTRASOUND_CANCEL";
        
        // 叫号
        case CommandID::CALL_NEXT: return "CALL_NEXT";
        case CommandID::CALL_SPECIFIC: return "CALL_SPECIFIC";
        case CommandID::RECALL: return "RECALL";
        case CommandID::SKIP: return "SKIP";
        
        // B超室
        case CommandID::ULTRASOUND_STATUS: return "ULTRASOUND_STATUS";
        case CommandID::ULTRASOUND_READY: return "ULTRASOUND_READY";
        case CommandID::ULTRASOUND_COMPLETE: return "ULTRASOUND_COMPLETE";
        
        // 管理员
        case CommandID::STATISTICS: return "STATISTICS";
        case CommandID::STATISTICS_DAILY: return "STATISTICS_DAILY";
        case CommandID::STATISTICS_MONTHLY: return "STATISTICS_MONTHLY";
        case CommandID::CONFIG_GET: return "CONFIG_GET";
        case CommandID::CONFIG_SET: return "CONFIG_SET";
        case CommandID::DOCTOR_ADD: return "DOCTOR_ADD";
        case CommandID::DOCTOR_UPDATE: return "DOCTOR_UPDATE";
        case CommandID::DOCTOR_DELETE: return "DOCTOR_DELETE";
        case CommandID::REPORT_EXPORT: return "REPORT_EXPORT";
        
        // 系统
        case CommandID::PING: return "PING";
        case CommandID::PONG: return "PONG";
        case CommandID::SERVER_INFO: return "SERVER_INFO";
        case CommandID::VERSION_CHECK: return "VERSION_CHECK";
        
        default: return "UNKNOWN";
    }
}

// =============================================================================
// 命令分类
// =============================================================================
enum class CommandCategory {
    Patient,      // 患者端命令
    Doctor,       // 医生端命令
    Display,      // 叫号/显示命令
    Ultrasound,   // B超室命令
    Admin,       // 管理员命令
    System        // 系统命令
};

SMARTSCHED_NODISCARD
inline CommandCategory getCommandCategory(CommandID cmd) {
    auto id = static_cast<uint16_t>(cmd);
    if (id >= 1001 && id <= 1032) return CommandCategory::Patient;
    if (id >= 2001 && id <= 2022) return CommandCategory::Doctor;
    if (id >= 3001 && id <= 3004) return CommandCategory::Display;
    if (id >= 4001 && id <= 4003) return CommandCategory::Ultrasound;
    if (id >= 5001 && id <= 5201) return CommandCategory::Admin;
    return CommandCategory::System;
}

// =============================================================================
// 命令权限级别
// =============================================================================
enum class CommandPermission {
    Public,      // 公开（无需登录）
    Patient,      // 患者权限
    Doctor,       // 医生权限
    Admin         // 管理员权限
};

SMARTSCHED_NODISCARD
inline CommandPermission getCommandPermission(CommandID cmd) {
    auto id = static_cast<uint16_t>(cmd);
    
    // 公开命令
    if (id == 1001 || id == 1002 || id == 1011 || id == 1012 || 
        id == 1013 || id == 1023 || id == 1031 || id == 1032 ||
        id == 9001 || id == 9002 || id == 9011 || id == 9012) {
        return CommandPermission::Public;
    }
    
    // 患者命令
    if (id == 1021 || id == 1022) {
        return CommandPermission::Patient;
    }
    
    // 医生命令
    if (id >= 2001 && id <= 2022) {
        return CommandPermission::Doctor;
    }
    
    // 管理员命令
    return CommandPermission::Admin;
}

} // namespace protocol
} // namespace smartsched
