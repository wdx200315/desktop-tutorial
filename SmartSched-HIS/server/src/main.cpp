/**
 * @file main.cpp
 * @brief SmartSched-HIS 服务端主程序入口
 * 
 * 智序医院门诊智慧调度系统 - 服务端
 * 
 * 功能:
 * - 多线程TCP服务器
 * - AES-256加密通信
 * - MySQL数据库
 * - 挂号、排队、就诊、B超调度
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDateTime>
#include <csignal>
#include <iostream>

#include "smartsched/server/tcpserver.h"
#include "smartsched/server/crypto.h"
#include "smartsched/server/database.h"
#include "smartsched/server/service.h"
#include "../../common/include/smartsched/utils/logger.h"
#include "../../common/include/smartsched/common/version.h"

using namespace smartsched;
using namespace smartsched::server;
using namespace smartsched::utils;

// =============================================================================
// 全局变量
// =============================================================================
static TcpServer* g_tcpServer = nullptr;
static BusinessService* g_businessService = nullptr;
static ConnectionPool* g_dbPool = nullptr;
static bool g_running = true;

// =============================================================================
// 信号处理
// =============================================================================
void signalHandler(int signum) {
    std::cout << "\n收到信号 " << signum << ", 正在关闭服务器..." << std::endl;
    g_running = false;
    
    if (g_tcpServer) {
        g_tcpServer->stop();
    }
}

// =============================================================================
// 配置加载
// =============================================================================
DatabaseConfig loadDatabaseConfig(const QString& config_file) {
    DatabaseConfig config;
    
    // 从配置文件加载（简化实现）
    // 实际应该从JSON/YAML文件读取
    
    config.host = "localhost";
    config.port = 3306;
    config.database = "smartsched";
    config.username = "root";
    config.password = "root";
    config.min_connections = 2;
    config.max_connections = 10;
    
    return config;
}

TcpServerConfig loadServerConfig() {
    TcpServerConfig config;
    
    config.port = 8888;
    config.bindAddress = "0.0.0.0";
    config.minThreads = 4;
    config.maxThreads = 16;
    config.maxConnections = 1000;
    config.connectionTimeout = 300;
    config.heartbeatInterval = 30;
    config.enableHeartbeat = true;
    
    return config;
}

// =============================================================================
// 数据库初始化
// =============================================================================
bool initializeDatabase(ConnectionPool* pool) {
    std::cout << "初始化数据库..." << std::endl;
    
    auto conn = pool->getConnection();
    if (!conn) {
        std::cerr << "无法获取数据库连接" << std::endl;
        return false;
    }
    
    // 创建科室表
    const char* create_departments = R"(
        CREATE TABLE IF NOT EXISTS departments (
            dept_id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            description TEXT,
            queue_capacity INT DEFAULT 50,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 创建医生表
    const char* create_doctors = R"(
        CREATE TABLE IF NOT EXISTS doctors (
            doctor_id INT AUTO_INCREMENT PRIMARY KEY,
            dept_id INT NOT NULL,
            name VARCHAR(50) NOT NULL,
            title VARCHAR(50),
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (dept_id) REFERENCES departments(dept_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 创建患者表
    const char* create_patients = R"(
        CREATE TABLE IF NOT EXISTS patients (
            patient_id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(50) NOT NULL,
            age INT,
            gender VARCHAR(10),
            phone VARCHAR(20),
            id_card VARCHAR(20),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 创建排队队列表
    const char* create_queue = R"(
        CREATE TABLE IF NOT EXISTS queue (
            queue_id INT AUTO_INCREMENT PRIMARY KEY,
            patient_id INT NOT NULL,
            dept_id INT NOT NULL,
            doctor_id INT,
            queue_number VARCHAR(20) NOT NULL,
            position INT,
            join_time BIGINT,
            estimated_start_time BIGINT,
            status INT DEFAULT 0,
            FOREIGN KEY (patient_id) REFERENCES patients(patient_id),
            FOREIGN KEY (dept_id) REFERENCES departments(dept_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 创建就诊记录表
    const char* create_records = R"(
        CREATE TABLE IF NOT EXISTS records (
            record_id INT AUTO_INCREMENT PRIMARY KEY,
            patient_id INT NOT NULL,
            doctor_id INT NOT NULL,
            start_time BIGINT,
            end_time BIGINT,
            duration INT,
            diagnosis TEXT,
            need_ultrasound BOOLEAN DEFAULT FALSE,
            FOREIGN KEY (patient_id) REFERENCES patients(patient_id),
            FOREIGN KEY (doctor_id) REFERENCES doctors(doctor_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 创建B超记录表
    const char* create_ultrasound = R"(
        CREATE TABLE IF NOT EXISTS ultrasound (
            appt_id INT AUTO_INCREMENT PRIMARY KEY,
            patient_id INT NOT NULL,
            machine_id INT NOT NULL,
            appointment_time BIGINT,
            start_time BIGINT,
            end_time BIGINT,
            status INT DEFAULT 0,
            FOREIGN KEY (patient_id) REFERENCES patients(patient_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )";
    
    // 执行建表
    conn->execute(create_departments);
    conn->execute(create_doctors);
    conn->execute(create_patients);
    conn->execute(create_queue);
    conn->execute(create_records);
    conn->execute(create_ultrasound);
    
    // 插入示例数据
    conn->execute("INSERT IGNORE INTO departments (dept_id, name, description) VALUES "
                  "(1, '内科', 'Internal Medicine'), "
                  "(2, '外科', 'Surgery'), "
                  "(3, '儿科', 'Pediatrics'), "
                  "(4, '妇科', 'Gynecology'), "
                  "(5, '骨科', 'Orthopedics')");
    
    conn->execute("INSERT IGNORE INTO doctors (doctor_id, dept_id, name, title) VALUES "
                  "(1, 1, '张医生', '主任医师'), "
                  "(2, 1, '李医生', '副主任医师'), "
                  "(3, 1, '王医生', '主治医师'), "
                  "(4, 2, '赵医生', '主任医师'), "
                  "(5, 3, '孙医生', '副主任医师')");
    
    std::cout << "数据库初始化完成" << std::endl;
    return true;
}

// =============================================================================
// 主函数
// =============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // 应用信息
    QCoreApplication::setApplicationName("SmartSched-HIS Server");
    QCoreApplication::setApplicationVersion(SMARTSCHED_VERSION_STRING);
    
    std::cout << "========================================" << std::endl;
    std::cout << " 智序医院门诊智慧调度系统 (SmartSched-HIS)" << std::endl;
    std::cout << " 版本: " << SMARTSCHED_VERSION_STRING << std::endl;
    std::cout << "========================================" << std::endl;
    
    // =====================================================================
    // 命令行参数解析
    // =====================================================================
    QCommandLineParser parser;
    parser.setApplicationDescription("SmartSched-HIS Server - 医院门诊智慧调度系统");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 端口
    QCommandLineOption portOption("p", "服务器端口", "port", "8888");
    parser.addOption(portOption);
    
    // 数据库
    QCommandLineOption dbHostOption("h", "数据库主机", "host", "localhost");
    parser.addOption(dbHostOption);
    
    QCommandLineOption dbPortOption("P", "数据库端口", "port", "3306");
    parser.addOption(dbPortOption);
    
    QCommandLineOption dbNameOption("d", "数据库名称", "name", "smartsched");
    parser.addOption(dbNameOption);
    
    QCommandLineOption dbUserOption("u", "数据库用户名", "user", "root");
    parser.addOption(dbUserOption);
    
    QCommandLineOption dbPassOption("w", "数据库密码", "password", "");
    parser.addOption(dbPassOption);
    
    // 日志级别
    QCommandLineOption logLevelOption("l", "日志级别 (trace|debug|info|warn|error)", "level", "info");
    parser.addOption(logLevelOption);
    
    // 解析参数
    parser.process(app);
    
    // =====================================================================
    // 日志初始化
    // =====================================================================
    Logger& logger = Logger::instance();
    logger.setOutput(LogOutput::Both);
    logger.addFileSink("logs/server.log");
    
    QString logLevel = parser.value(logLevelOption);
    if (logLevel == "trace") logger.setLevel(LogLevel::Trace);
    else if (logLevel == "debug") logger.setLevel(LogLevel::Debug);
    else if (logLevel == "warn") logger.setLevel(LogLevel::Warn);
    else if (logLevel == "error") logger.setLevel(LogLevel::Error);
    else logger.setLevel(LogLevel::Info);
    
    LOG_INFO("日志系统初始化完成");
    
    // =====================================================================
    // 信号处理
    // =====================================================================
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // =====================================================================
    // 数据库初始化
    // =====================================================================
    DatabaseConfig dbConfig;
    dbConfig.host = parser.value(dbHostOption).toStdString();
    dbConfig.port = parser.value(dbPortOption).toInt();
    dbConfig.database = parser.value(dbNameOption).toStdString();
    dbConfig.username = parser.value(dbUserOption).toStdString();
    dbConfig.password = parser.value(dbPassOption).toStdString();
    
    g_dbPool = new ConnectionPool(dbConfig);
    
    if (!g_dbPool->initialize()) {
        std::cerr << "数据库连接失败，请检查配置" << std::endl;
        LOG_FATAL("数据库连接失败");
        return 1;
    }
    
    // 初始化数据库表结构
    if (!initializeDatabase(g_dbPool)) {
        std::cerr << "数据库初始化失败" << std::endl;
        LOG_FATAL("数据库初始化失败");
        return 1;
    }
    
    // =====================================================================
    // 业务服务初始化
    // =====================================================================
    g_businessService = new BusinessService(
        std::shared_ptr<ConnectionPool>(g_dbPool), &app
    );
    
    if (!g_businessService->initialize()) {
        std::cerr << "业务服务初始化失败" << std::endl;
        LOG_FATAL("业务服务初始化失败");
        return 1;
    }
    
    // =====================================================================
    // TCP服务器初始化
    // =====================================================================
    TcpServerConfig serverConfig;
    serverConfig.port = parser.value(portOption).toInt();
    
    g_tcpServer = new TcpServer(&app);
    g_tcpServer->setConfig(serverConfig);
    
    // 创建消息路由器
    MessageRouter* router = new MessageRouter(g_businessService, &app);
    g_tcpServer->setMessageHandler(router);
    
    // 消息响应信号连接
    QObject::connect(router, &MessageRouter::responseReady,
                    [g_tcpServer](const QString& conn_id, const QString& message) {
        g_tcpServer->sendToClient(conn_id, message);
    });
    
    // =====================================================================
    // 启动服务器
    // =====================================================================
    if (!g_tcpServer->start()) {
        std::cerr << "服务器启动失败" << std::endl;
        LOG_FATAL("服务器启动失败");
        return 1;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << " 服务器已启动" << std::endl;
    std::cout << " 监听端口: " << serverConfig.port << std::endl;
    std::cout << " 数据库: " << dbConfig.host << ":" << dbConfig.port << "/" << dbConfig.database << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << " 按 Ctrl+C 停止服务器" << std::endl;
    std::cout << std::endl;
    
    LOG_INFO("SmartSched-HIS Server 启动完成");
    
    // =====================================================================
    // 主事件循环
    // =====================================================================
    while (g_running) {
        app.processEvents(QEventLoop::WaitForMoreEvents);
        
        // 定期保存统计信息
        static int save_counter = 0;
        if (++save_counter >= 600) {  // 每10分钟
            LOG_INFO("当前连接数: " + std::to_string(g_tcpServer->connectionCount()));
            save_counter = 0;
        }
    }
    
    // =====================================================================
    // 清理
    // =====================================================================
    LOG_INFO("正在关闭服务器...");
    
    g_businessService->shutdown();
    g_tcpServer->stop();
    g_dbPool->shutdown();
    
    std::cout << "服务器已关闭" << std::endl;
    LOG_INFO("SmartSched-HIS Server 已关闭");
    
    return 0;
}
