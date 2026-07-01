# SmartSched-HIS API 文档

## 目录
- [概述](#概述)
- [认证](#认证)
- [患者端API](#患者端api)
- [医生端API](#医生端api)
- [管理端API](#管理端api)
- [通用API](#通用api)
- [错误码](#错误码)
- [数据模型](#数据模型)

---

## 概述

### 协议格式

所有请求和响应均使用JSON格式，消息以换行符 `\n` 分隔。

**请求格式：**
```json
{
    "cmd": 命令码,
    "seq": 序列号,
    "token": "JWT令牌",
    "params": {
        "key1": "value1",
        "key2": "value2"
    }
}
```

**响应格式：**
```json
{
    "cmd": 命令码,
    "seq": 序列号,
    "ret": 结果码,
    "data": {
        "key1": "value1"
    },
    "msg": "错误信息"
}
```

### 连接地址
- 生产环境：`https://api.smartsched.example.com`
- 测试环境：`http://localhost:8888`

---

## 认证

### 医生登录

```
POST /api/doctor/login
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| doctor_id | string | 是 | 医生工号 |
| password | string | 是 | 密码（MD5加密） |

**响应示例：**
```json
{
    "cmd": 101,
    "seq": 1001,
    "ret": 0,
    "data": {
        "token": "eyJhbGciOiJIUzI1NiJ9...",
        "doctor_name": "李明",
        "department": "内科",
        "department_id": 1,
        "expires_in": 3600
    }
}
```

### 管理员登录

```
POST /api/admin/login
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| username | string | 是 | 用户名 |
| password | string | 是 | 密码 |
| device_id | string | 否 | 设备ID |

**响应示例：**
```json
{
    "cmd": 201,
    "seq": 1002,
    "ret": 0,
    "data": {
        "token": "eyJhbGciOiJIUzI1NiJ9...",
        "role": "admin",
        "permissions": ["system_config", "user_manage"],
        "expires_in": 86400
    }
}
```

### 令牌刷新

```
POST /api/auth/refresh
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| refresh_token | string | 是 | 刷新令牌 |

---

## 患者端API

### 01 - 患者挂号

```
POST /api/patient/register
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| patient_id | string | 是 | 患者ID/卡号 |
| department_id | int | 是 | 科室ID |
| patient_name | string | 是 | 患者姓名 |
| id_card | string | 否 | 身份证号（加密传输） |
| phone | string | 否 | 联系电话 |
| notes | string | 否 | 备注信息 |

**响应示例：**
```json
{
    "cmd": 1,
    "seq": 1003,
    "ret": 0,
    "data": {
        "registration_id": "R20240115100001",
        "queue_number": 15,
        "department_name": "内科",
        "doctor_name": "李明医生",
        "estimated_wait_time": 30,
        "register_time": "2024-01-15 09:30:00",
        "position": 15
    }
}
```

### 02 - 查询排队状态

```
GET /api/patient/queue-status
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| patient_id | string | 是 | 患者ID |
| registration_id | string | 否 | 挂号ID（二选一） |

**响应示例：**
```json
{
    "cmd": 2,
    "seq": 1004,
    "ret": 0,
    "data": {
        "queue_number": 15,
        "position": 3,
        "ahead_count": 2,
        "estimated_time": 15,
        "status": "waiting",
        "current_queue": [
            {"queue_number": 13, "status": "called"},
            {"queue_number": 14, "status": "consulting"},
            {"queue_number": 15, "status": "waiting"}
        ]
    }
}
```

### 03 - 取消挂号

```
POST /api/patient/cancel
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| registration_id | string | 是 | 挂号ID |

**响应示例：**
```json
{
    "cmd": 3,
    "seq": 1005,
    "ret": 0,
    "data": {
        "refund_amount": 0,
        "message": "退号成功"
    }
}
```

### 04 - 获取我的挂号记录

```
GET /api/patient/registrations
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| patient_id | string | 是 | 患者ID |
| date | string | 否 | 日期（YYYY-MM-DD） |
| status | string | 否 | 状态筛选 |
| page | int | 否 | 页码（默认1） |
| page_size | int | 否 | 每页数量（默认10） |

**响应示例：**
```json
{
    "cmd": 4,
    "seq": 1006,
    "ret": 0,
    "data": {
        "total": 25,
        "page": 1,
        "page_size": 10,
        "items": [
            {
                "registration_id": "R20240115100001",
                "department_name": "内科",
                "doctor_name": "李明医生",
                "register_time": "2024-01-15 09:30:00",
                "status": "completed",
                "diagnosis": "感冒"
            }
        ]
    }
}
```

---

## 医生端API

### 101 - 医生叫号

```
POST /api/doctor/call
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| queue_number | int | 是 | 排队号 |
| doctor_id | int | 是 | 医生ID（可从Token获取） |

**响应示例：**
```json
{
    "cmd": 102,
    "seq": 2001,
    "ret": 0,
    "data": {
        "patient_name": "张三",
        "registration_id": "R20240115100001",
        "wait_time": 25,
        "call_time": "2024-01-15 10:30:00"
    }
}
```

### 102 - 完成诊疗

```
POST /api/doctor/finish
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| consultation_id | string | 是 | 诊疗记录ID |
| diagnosis | string | 是 | 诊断结果 |
| prescription | string | 否 | 处方信息 |
| advice | string | 否 | 医嘱 |
| next_visit | string | 否 | 复诊日期 |

**响应示例：**
```json
{
    "cmd": 103,
    "seq": 2002,
    "ret": 0,
    "data": {
        "next_queue_number": 16,
        "today_count": 28,
        "avg_time": 8.5
    }
}
```

### 103 - 获取本科室队列

```
GET /api/doctor/queue
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| doctor_id | int | 是 | 医生ID |

**响应示例：**
```json
{
    "cmd": 104,
    "seq": 2003,
    "ret": 0,
    "data": {
        "total_waiting": 12,
        "queue": [
            {"queue_number": 16, "patient_name": "张三", "wait_time": 5, "status": "waiting"},
            {"queue_number": 17, "patient_name": "李四", "wait_time": 0, "status": "waiting"}
        ]
    }
}
```

### 104 - 转诊

```
POST /api/doctor/transfer
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| registration_id | string | 是 | 挂号ID |
| target_department_id | int | 是 | 目标科室ID |
| reason | string | 否 | 转诊原因 |

---

## 管理端API

### 201 - 获取统计数据

```
GET /api/admin/statistics
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | 是 | 统计类型：daily/monthly/yearly |
| date | string | 是 | 日期 |
| department_id | int | 否 | 科室筛选 |

**响应示例：**
```json
{
    "cmd": 202,
    "seq": 3001,
    "ret": 0,
    "data": {
        "total_registrations": 520,
        "total_completed": 480,
        "avg_wait_time": 22,
        "peak_queue_size": 65,
        "department_stats": [
            {"department_id": 1, "name": "内科", "count": 120},
            {"department_id": 2, "name": "外科", "count": 85}
        ]
    }
}
```

### 202 - 导出数据

```
POST /api/admin/export
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | 是 | 导出类型：daily/monthly/custom |
| format | string | 是 | 格式：csv/excel/json |
| start_date | string | 是 | 开始日期 |
| end_date | string | 是 | 结束日期 |
| filters | object | 否 | 筛选条件 |

**响应：** 返回文件下载链接或直接流

### 203 - 管理系统配置

```
GET /api/admin/config
POST /api/admin/config
```

**配置项：**
```json
{
    "business": {
        "queue_capacity": 1000,
        "max_wait_time": 120,
        "call_timeout": 300,
        "auto_reassign": true
    },
    "notification": {
        "sound_enabled": true,
        "display_auto_refresh": true,
        "refresh_interval": 5
    }
}
```

---

## 通用API

### 500 - 获取科室列表

```
GET /api/common/departments
```

**响应示例：**
```json
{
    "cmd": 500,
    "seq": 1,
    "ret": 0,
    "data": {
        "departments": [
            {"id": 1, "name": "内科", "floor": 2, "doctors": 5},
            {"id": 2, "name": "外科", "floor": 3, "doctors": 4}
        ]
    }
}
```

### 501 - 获取医生排班

```
GET /api/common/schedule
```

**请求参数：**
| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| department_id | int | 否 | 科室ID |
| date | string | 否 | 日期 |

### 502 - 获取公告

```
GET /api/common/announcements
```

---

## 错误码

| 错误码 | 常量 | 说明 |
|--------|------|------|
| 0 | RET_SUCCESS | 成功 |
| -1 | RET_INVALID_PARAMS | 参数错误 |
| -2 | RET_NOT_FOUND | 资源不存在 |
| -3 | RET_PERMISSION_DENIED | 权限不足 |
| -4 | RET_ALREADY_EXISTS | 资源已存在 |
| -5 | RET_QUEUE_FULL | 队列已满 |
| -6 | RET_DEPARTMENT_CLOSED | 科室已关闭 |
| -10 | RET_INVALID_TOKEN | Token无效 |
| -11 | RET_TOKEN_EXPIRED | Token过期 |
| -12 | RET_LOGIN_FAILED | 登录失败 |
| -99 | RET_SERVER_ERROR | 服务器内部错误 |

---

## 数据模型

### 挂号记录 (Registration)

```json
{
    "registration_id": "R20240115100001",
    "patient_id": "P001",
    "patient_name": "张三",
    "department_id": 1,
    "department_name": "内科",
    "doctor_id": 101,
    "doctor_name": "李明医生",
    "queue_number": 15,
    "status": "waiting",
    "register_time": "2024-01-15 09:30:00",
    "call_time": null,
    "finish_time": null
}
```

### 诊疗记录 (Consultation)

```json
{
    "consultation_id": "C20240115001",
    "registration_id": "R20240115100001",
    "doctor_id": 101,
    "diagnosis": "急性上呼吸道感染",
    "prescription": "感冒灵颗粒 x 3",
    "advice": "多喝水，注意休息",
    "call_time": "2024-01-15 10:30:00",
    "finish_time": "2024-01-15 10:42:00",
    "duration": 12
}
```

### 队列状态 (QueueStatus)

```json
{
    "queue_number": 15,
    "patient_name": "张三",
    "department_id": 1,
    "status": "waiting",
    "register_time": "2024-01-15 09:30:00",
    "wait_time": 25,
    "position": 3
}
```

---

*文档版本: 1.0.0*
*最后更新: 2024-01-15*
