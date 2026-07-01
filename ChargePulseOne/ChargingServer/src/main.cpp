#include "common.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "ConnectionPool.h"
#include "DatabaseManager.h"
#include "SessionManager.h"
#include "TcpServer.h"
#include "ClientHandler.h"
#include "CryptoUtils.h"
#include "ChargingService.h"

int main() {
    // 初始化日志系统
    // 日志文件路径, 最大文件大小(MB), 最大备份文件数
    Logger::instance().setFile("logs/chargepulse.log", 10, 5);
    Logger::instance().setLevel(INFO);  // 默认INFO级别
    Logger::instance().log(INFO, "========================================");
    Logger::instance().log(INFO, "ChargePulse One Server starting...");
    Logger::instance().log(INFO, "========================================");

    // 加载配置
    ConfigManager::instance().load("config/server.enc");
    
    // 初始化数据库连接池
    try {
        ConnectionPool::instance().init(
            ConfigManager::instance().getDBHost(),
            ConfigManager::instance().getDBUser(),
            ConfigManager::instance().getDBPass(),
            ConfigManager::instance().getDBName(),
            ConfigManager::instance().getDBPort(),
            10
        );
    } catch(std::exception& e) {
        Logger::instance().log(ERROR, "DB init failed: " + std::string(e.what()));
        return 1;
    }

    // 建表（顺序重要）
    Logger::instance().log(INFO, "Creating tables...");
    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(50) UNIQUE NOT NULL,
            password VARCHAR(64) NOT NULL,
            role ENUM('driver','operator','admin') DEFAULT 'driver',
            phone VARCHAR(20),
            plate_number VARCHAR(20),
            balance DECIMAL(10,2) DEFAULT 0,
            status ENUM('active','disabled','blacklisted') DEFAULT 'active',
            device_token VARCHAR(255),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS chargers (
            id INT AUTO_INCREMENT PRIMARY KEY,
            serial_number VARCHAR(50) UNIQUE NOT NULL,
            status ENUM('online','offline','charging','fault') DEFAULT 'offline',
            power_kw DECIMAL(10,2) DEFAULT 60.0,
            temperature DECIMAL(5,2) DEFAULT 25.0,
            location VARCHAR(200),
            health_score INT DEFAULT 100,
            last_heartbeat TIMESTAMP NULL
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS charging_orders (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            charger_id INT NOT NULL,
            mode VARCHAR(20),
            target_value DECIMAL(10,2),
            start_time TIMESTAMP NULL,
            end_time TIMESTAMP NULL,
            energy_kwh DECIMAL(10,2) DEFAULT 0,
            amount DECIMAL(10,2) DEFAULT 0,
            status ENUM('charging','completed','cancelled') DEFAULT 'charging'
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS rates (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100),
            price_kwh DECIMAL(10,2) NOT NULL DEFAULT 1.0,
            service_fee DECIMAL(10,2) NOT NULL DEFAULT 0.5,
            start_time TIME DEFAULT '00:00:00',
            end_time TIME DEFAULT '23:59:59'
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS alarms (
            id INT AUTO_INCREMENT PRIMARY KEY,
            charger_id INT,
            type VARCHAR(50),
            level ENUM('info','warning','critical') DEFAULT 'warning',
            message TEXT,
            status ENUM('active','resolved') DEFAULT 'active',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            resolved_at TIMESTAMP NULL,
            note TEXT
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS reservations (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            charger_id INT NOT NULL,
            reserve_time DATETIME NOT NULL,
            mode VARCHAR(20),
            target_value DECIMAL(10,2),
            status ENUM('pending','executed','cancelled') DEFAULT 'pending',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS payments (
            id INT AUTO_INCREMENT PRIMARY KEY,
            order_id INT NOT NULL,
            user_id INT NOT NULL,
            amount DECIMAL(10,2) NOT NULL,
            status ENUM('pending','paid','failed','refunded') DEFAULT 'pending',
            paid_at TIMESTAMP NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            title VARCHAR(200),
            content TEXT,
            is_read TINYINT DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS coupons (
            id INT AUTO_INCREMENT PRIMARY KEY,
            code VARCHAR(50) UNIQUE NOT NULL,
            type ENUM('fixed','discount') DEFAULT 'fixed',
            value DECIMAL(10,2) NOT NULL,
            min_order_amount DECIMAL(10,2) DEFAULT 0,
            valid_from DATE,
            valid_to DATE,
            quantity INT DEFAULT 0,
            status ENUM('active','inactive') DEFAULT 'active'
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS user_coupons (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            coupon_id INT NOT NULL,
            status ENUM('unused','used','expired') DEFAULT 'unused',
            used_at TIMESTAMP NULL
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS member_levels (
            id INT AUTO_INCREMENT PRIMARY KEY,
            level VARCHAR(20) UNIQUE NOT NULL,
            name VARCHAR(50),
            min_amount DECIMAL(10,2) NOT NULL,
            discount DECIMAL(3,2) DEFAULT 1.00,
            perks TEXT
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");
    DatabaseManager::instance().execute("INSERT IGNORE INTO member_levels (level, name, min_amount, discount, perks) VALUES ('normal','普通会员',0,1.00,'')");
    DatabaseManager::instance().execute("INSERT IGNORE INTO member_levels (level, name, min_amount, discount, perks) VALUES ('silver','白银会员',500,0.98,'积分加速')");
    DatabaseManager::instance().execute("INSERT IGNORE INTO member_levels (level, name, min_amount, discount, perks) VALUES ('gold','黄金会员',1000,0.95,'专属客服')");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS sys_config (
            id INT AUTO_INCREMENT PRIMARY KEY,
            `key` VARCHAR(50) UNIQUE NOT NULL,
            `value` TEXT
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS operation_logs (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT,
            type VARCHAR(50),
            description TEXT,
            ip VARCHAR(45),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS vehicles (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            plate_number VARCHAR(20) NOT NULL,
            brand VARCHAR(50),
            model VARCHAR(50),
            vin VARCHAR(50),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    DatabaseManager::instance().execute(R"(
        CREATE TABLE IF NOT EXISTS login_logs (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT,
            ip VARCHAR(45),
            login_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    )");

    Logger::instance().log(INFO, "Tables initialized");

    // ========== 初始化默认数据 ==========
    Logger::instance().log(INFO, "Initializing default data...");

    // 插入默认管理员用户 (密码: admin123)
    std::string adminHash = CryptoUtils::sha256("admin123");
    std::string adminSql = "INSERT IGNORE INTO users (username, password, role, phone, balance) "
                           "VALUES ('admin', '" + adminHash + "', 'admin', '13800138000', 1000.00)";
    DatabaseManager::instance().execute(adminSql);

    // 插入测试充电桩
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO chargers (serial_number, status, power_kw, location) VALUES ('CP-001', 'online', 60.0, 'A区1号充电站')"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO chargers (serial_number, status, power_kw, location) VALUES ('CP-002', 'online', 120.0, 'A区1号充电站')"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO chargers (serial_number, status, power_kw, location) VALUES ('CP-003', 'offline', 60.0, 'B区2号充电站')"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO chargers (serial_number, status, power_kw, location) VALUES ('CP-004', 'charging', 180.0, 'C区3号充电站')"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO chargers (serial_number, status, power_kw, location) VALUES ('CP-005', 'online', 60.0, 'D区4号充电站')"
    );

    // 插入默认费率
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO rates (name, price_kwh, service_fee, start_time, end_time) "
        "VALUES ('标准电价', 1.20, 0.50, '00:00:00', '23:59:59')"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO rates (name, price_kwh, service_fee, start_time, end_time) "
        "VALUES ('尖时电价', 1.80, 0.60, '08:00:00', '11:00:00')"
    );

    // 插入默认优惠券
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO coupons (code, type, value, min_order_amount, valid_from, valid_to, quantity) "
        "VALUES ('WELCOME10', 'fixed', 10.00, 50.00, '2026-01-01', '2026-12-31', 1000)"
    );
    DatabaseManager::instance().execute(
        "INSERT IGNORE INTO coupons (code, type, value, min_order_amount, valid_from, valid_to, quantity) "
        "VALUES ('SUMMER20', 'discount', 0.80, 100.00, '2026-06-01', '2026-08-31', 500)"
    );

    Logger::instance().log(INFO, "Default data initialized");

    // 启动 TCP 服务器
    TcpServer server;
    ClientHandler handler(server);
    
    if(!server.start(ConfigManager::instance().getServerPort())) {
        Logger::instance().log(ERROR, "Failed to start TCP server");
        return 1;
    }

    Logger::instance().log(INFO, "Server is running on port " + std::to_string(ConfigManager::instance().getServerPort()));

    // 主循环
    int tickCount = 0;
    while(true) {
        sleep_ms(1000); // 每秒执行一次
        tickCount++;

        // 更新充电模拟数据 (每秒)
        updateChargeSimulation();

        // 清理过期会话 (每60秒)
        if (tickCount % 60 == 0) {
            SessionManager::instance().cleanExpired();
            Logger::instance().log(INFO, "Session cleanup, active sessions: " +
                                  std::to_string(SessionManager::instance().getActiveCount()));
        }
    }

    server.stop();
    ConnectionPool::instance().destroy();
    return 0;
}
