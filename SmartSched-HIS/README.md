# 智序医院门诊智慧调度系统 (SmartSched-HIS)

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![C++](https://img.shields.io/badge/C++-17-green)
![Qt](https://img.shields.io/badge/Qt-6.11.1-purple)
![License](https://img.shields.io/badge/license-MIT-orange)

## 项目概述

**SmartSched-HIS** 是一款面向医院门诊的企业级智慧调度系统，采用C/S架构，实现挂号、排队、就诊、B超检查的全流程数字化管理。

### 核心特性

- 🎯 **多角色支持**：患者、医生、护士、管理员
- 🔒 **高安全性**：AES-256加密通信、JWT认证、RBAC权限控制
- ⚡ **高并发**：多线程TCP服务器、线程池管理、LRU缓存
- 🌐 **跨平台**：支持Windows、Linux（含麒麟系统）
- 📊 **实时看板**：科室排队状态实时展示
- 🛡️ **安全防护**：SQL注入检测、XSS防护、CSRF令牌
- 📈 **数据分析**：日报/月报/年报、趋势分析、CSV导出
- 🔄 **集群支持**：双机热备、故障转移、分布式锁

## 技术架构

### 技术栈

| 层级 | 技术选型 | 版本 |
|------|---------|------|
| 服务端语言 | C++ | C++17 |
| 服务端通信 | TCP + JSON | 自定义协议 |
| 服务端加密 | OpenSSL | AES-256-CBC |
| 服务端数据库 | MySQL | 8.0+ |
| 客户端框架 | Qt | 6.11.1 |
| 构建系统 | CMake | 3.16+ |
| 单元测试 | Google Test | 1.14.0 |

### 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        客户端层                              │
├─────────────┬─────────────┬─────────────┬──────────────────┤
│  患者挂号终端 │  医生工作站  │   排队看板    │      管理端       │
│   (QML)     │  (Widgets)  │   (QML)     │    (Widgets)     │
└──────┬──────┴──────┬──────┴──────┬──────┴────────┬─────────┘
       │             │             │               │
       │    TCP + JSON + \n 分隔符     │               │
       │             │             │               │
┌──────▼─────────────▼─────────────▼───────────────▼─────────┐
│                        服务端层                              │
├─────────────┬─────────────┬─────────────┬──────────────────┤
│  网络监听线程 │  工作线程池  │  业务逻辑层   │   数据库连接池    │
│  (QTcpServer)│ (QThreadPool)│  (挂号/排队)  │  (MySQL C API)  │
└─────────────┴─────────────┴─────────────┴──────────────────┤
       │                                                   │
       │              AES-256-CBC + JWT 认证              │
       └───────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────────┐
                    │    MySQL 8.0+      │
                    │  科室/医生/患者/排队  │
                    └─────────────────────┘
```

## 项目结构

```
SmartSched-HIS/
├── CMakeLists.txt           # 顶层构建配置
│
├── common/                  # 公共模块
│   ├── include/smartsched/
│   │   ├── common/         # 编译器、版本、宏定义
│   │   ├── protocol/       # 通信协议定义（55个命令）
│   │   ├── crypto/         # AES-256加密工具
│   │   ├── utils/          # 工具类
│   │   │   ├── json.h/cpp  # JSON解析
│   │   │   ├── datetime.h  # 时间处理
│   │   │   ├── bytebuffer.h # 字节缓冲
│   │   │   ├── cache.h     # LRU缓存+内存池
│   │   │   ├── config.h/cpp # 配置管理
│   │   │   └── filelogger.h # 文件日志
│   │   ├── security/       # 安全模块
│   │   │   ├── auth.h/cpp  # JWT认证+RBAC
│   │   │   └── security.h/cpp # SQL/XSS防护
│   │   └── cluster/        # 集群模块
│   │       └── cluster.h   # 双机热备+分布式锁
│   └── src/
│       ├── logger.cpp
│       ├── utils/
│       ├── security/
│       └── crypto/
│
├── server/                  # 服务端
│   ├── include/smartsched/server/
│   │   ├── tcpserver.h     # 多线程TCP服务器
│   │   ├── crypto.h        # AES-256加密
│   │   ├── database.h      # MySQL连接池
│   │   ├── service.h/cpp   # 业务服务层
│   │   ├── statistics.h/cpp # 统计报表
│   │   └── statistics_service.h # 统计服务
│   ├── src/
│   │   ├── core/           # 网络核心
│   │   ├── crypto/         # 加密实现
│   │   ├── db/             # 数据库实现
│   │   ├── biz/            # 业务逻辑
│   │   └── main.cpp        # 服务端入口
│   └── CMakeLists.txt
│
├── client/                  # 客户端（Qt/QML）
│   ├── include/smartsched/client/
│   │   ├── networkclient.h  # 网络客户端
│   │   └── datamodel.h     # 数据模型
│   ├── src/
│   │   ├── base/           # 基础库
│   │   ├── patient/         # 患者终端
│   │   ├── doctor/          # 医生工作站
│   │   ├── display/         # 排队看板
│   │   └── admin/           # 管理端
│   ├── qml/                 # QML界面
│   │   ├── patient/
│   │   ├── doctor/
│   │   └── display/
│   └── CMakeLists.txt
│
├── tools/                   # 工具
│   └── stresstest/          # 压力测试工具
│
├── tests/                   # 测试
│   ├── test_common.cpp      # 公共模块测试
│   ├── test_protocol.cpp    # 协议测试
│   ├── test_crypto.cpp      # 加密测试
│   ├── test_auth.cpp        # 认证测试
│   ├── test_cache.cpp       # 缓存测试
│   ├── test_security.cpp    # 安全测试
│   └── test_integration.cpp  # 集成测试
│
├── doc/                     # 文档
│   ├── database/init.sql    # 数据库初始化
│   ├── api/API_DOC.md       # API文档
│   ├── manual/USER_MANUAL.md # 用户手册
│   └── deployment/DEPLOYMENT.md # 部署文档
│
├── docker/                  # Docker配置
│   ├── docker-compose.yml
│   ├── Dockerfile.server
│   ├── Dockerfile.client
│   └── prometheus.yml
│
├── deploy.sh                # 一键部署脚本
│
└── README.md                # 本文件
```

## 快速开始

### 环境要求

- **操作系统**: Windows 10/11 或 Ubuntu 22.04/24.04
- **编译器**: 
  - Linux: GCC 12+
  - Windows: MSVC 2022 或 MinGW
- **Qt**: 6.11.1
- **MySQL**: 8.0+
- **CMake**: 3.16+

### Linux 构建

```bash
# 安装依赖（Ubuntu）
sudo apt install build-essential cmake \
    qt6-base-dev qt6-network-dev \
    qt6-qml-dev qt6-quick-dev \
    libmysqlclient-dev libssl-dev

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 运行服务端
./bin/smartsched-server -p 8888
```

### Windows 构建

```powershell
# 使用 Visual Studio Developer Command Prompt
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# 运行
.\bin\Release\smartsched-server.exe -p 8888
```

### Docker 部署

```bash
# 一键部署
./deploy.sh --mode=production --db-password your_password

# 或使用Docker Compose
cd docker
docker-compose up -d
```

## 功能模块

### ✅ 已完成功能

#### 1. 患者端功能
- [x] 科室列表查询
- [x] 医生列表查询
- [x] 在线挂号
- [x] 排队状态查询
- [x] 预计等待时间
- [x] 取消挂号

#### 2. 医生端功能
- [x] 叫号（下一位/指定）
- [x] 开始接诊
- [x] 结束接诊
- [x] 暂停接诊
- [x] 患者信息查询
- [x] 历史记录查看
- [x] 申请B超

#### 3. B超室功能
- [x] B超预约管理
- [x] 机器状态监控
- [x] 完成检查

#### 4. 管理员功能
- [x] 医生增删改查
- [x] 统计数据报表
- [x] 日报/月报/年报导出
- [x] 系统配置

#### 5. 系统安全
- [x] AES-256-CBC加密通信
- [x] JWT Token认证
- [x] RBAC权限控制
- [x] SQL注入检测
- [x] XSS防护
- [x] CSRF令牌
- [x] 请求限流
- [x] IP黑名单

#### 6. 性能优化
- [x] MySQL连接池
- [x] LRU缓存
- [x] 内存池
- [x] 环形缓冲区
- [x] 文件日志轮转

#### 7. 集群支持
- [x] 主从选举
- [x] 故障检测
- [x] 数据同步
- [x] 分布式锁
- [x] 连接池管理

#### 8. 测试覆盖
- [x] 公共模块单元测试
- [x] 协议层测试
- [x] 加密模块测试
- [x] 认证授权测试
- [x] 缓存模块测试
- [x] 安全模块测试
- [x] 业务流程集成测试

## 通信协议

### 消息格式

```json
// 请求消息
{
    "cmd": 1,
    "seq": 12345,
    "token": "jwt_token",
    "patient_id": "P001",
    "department_id": 1
}

// 响应消息
{
    "cmd": 1,
    "seq": 12345,
    "ret": 0,
    "data": {
        "registration_id": "R20240115001",
        "queue_number": 15
    }
}
```

### 核心命令 (55个)

| 范围 | 命令ID | 名称 | 描述 |
|------|--------|------|------|
| 患者 | 1-50 | 挂号/查询/取消 | 患者操作 |
| 医生 | 101-150 | 叫号/完成/转诊 | 医生操作 |
| B超 | 201-220 | B超管理 | B超室操作 |
| 管理 | 301-350 | 配置/统计 | 管理员操作 |
| 通用 | 501-550 | 科室/排班 | 通用查询 |

## 性能指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 并发连接 | 1000+ | 单机支持 |
| 响应时间 | < 50ms | P99 |
| QPS | 5000+ | 压力测试 |
| CPU利用率 | < 70% | 峰值负载 |
| 内存占用 | < 500MB | 基准配置 |

## 开发进度

| 阶段 | 状态 | 说明 |
|------|------|------|
| 阶段一：需求分析 | ✅ 完成 | 需求文档、架构设计 |
| 阶段二：基础框架 | ✅ 完成 | CMake配置、模块划分 |
| 阶段三：网络加密 | ✅ 完成 | TCP服务器、AES加密 |
| 阶段四：核心业务 | ✅ 完成 | 挂号、排队、就诊 |
| 阶段五：客户端开发 | ✅ 完成 | Qt/QML四个客户端 |
| 阶段六：统计监控 | ✅ 完成 | 日报/月报、CSV导出 |
| 阶段七：性能安全 | ✅ 完成 | 认证、缓存、集群 |
| 阶段八：测试交付 | ✅ 完成 | 单元测试、文档 |

## 项目文档

| 文档 | 路径 | 说明 |
|------|------|------|
| API文档 | `doc/api/API_DOC.md` | 完整的API接口说明 |
| 用户手册 | `doc/manual/USER_MANUAL.md` | 各角色使用指南 |
| 部署文档 | `doc/deployment/DEPLOYMENT.md` | 详细部署步骤 |
| 数据库设计 | `doc/database/init.sql` | 表结构和初始化数据 |

## 扩展规划

### 已完成
- [x] Docker容器化
- [x] 一键部署脚本
- [x] 监控告警集成

### 短期扩展
- [ ] Web管理后台
- [ ] 移动端App

### 长期扩展
- [ ] AI智能调度算法
- [ ] 区块链病历存证
- [ ] 多院区协同

## 许可证

本项目采用 MIT 许可证。

## 联系方式

- 项目主页: https://github.com/smartsched/his
- 问题反馈: https://github.com/smartsched/his/issues
- 技术支持: support@smartsched.example.com

---

**智序** - 让医院调度更智慧
