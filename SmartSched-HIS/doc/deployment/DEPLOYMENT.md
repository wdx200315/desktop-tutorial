# 智序医院门诊智慧调度系统 - 部署文档

## 目录
- [系统要求](#系统要求)
- [快速部署](#快速部署)
- [详细安装步骤](#详细安装步骤)
- [配置说明](#配置说明)
- [Docker部署](#docker部署)
- [集群部署](#集群部署)
- [运维监控](#运维监控)
- [故障排查](#故障排查)
- [安全加固](#安全加固)

---

## 系统要求

### 硬件要求
| 组件 | 最低配置 | 推荐配置 |
|------|----------|----------|
| CPU | 4核 | 8核+ |
| 内存 | 8GB | 16GB+ |
| 磁盘 | 100GB SSD | 500GB SSD |
| 网络 | 100Mbps | 1Gbps |

### 软件要求
- **操作系统**: Ubuntu 20.04+ / CentOS 8+ / Windows Server 2019+
- **数据库**: MySQL 8.0+
- **运行时**: Qt 6.5+ (已编译二进制)
- **依赖库**: OpenSSL 1.1.1+, CMake 3.16+

---

## 快速部署

### 一键部署脚本 (Linux)

```bash
# 下载部署脚本
curl -O https://raw.githubusercontent.com/your-org/smartsched/main/deploy.sh
chmod +x deploy.sh

# 执行部署 (需要root权限)
sudo ./deploy.sh --mode=production --db-host=localhost --db-password=your_password

# 启动服务
sudo systemctl start smartsched-server
sudo systemctl enable smartsched-server
```

### Windows 快速安装

1. 下载最新 Release 版本
2. 运行安装程序 `SmartSched-Setup.exe`
3. 按照向导完成安装
4. 修改 `config/server.json` 中的数据库配置
5. 运行"智序服务端"快捷方式

---

## 详细安装步骤

### 步骤 1: 安装依赖

**Ubuntu/Debian:**
```bash
# 安装 MySQL
sudo apt update
sudo apt install -y mysql-server mysql-client

# 安装 OpenSSL
sudo apt install -y libssl-dev

# 安装 CMake
sudo apt install -y cmake build-essential
```

**CentOS/RHEL:**
```bash
# 安装 MySQL
sudo dnf install -y mysql-server mysql

# 安装 OpenSSL
sudo dnf install -y openssl-devel

# 安装 CMake
sudo dnf install -y cmake gcc-c++
```

**Windows:**
- 下载并安装 [MySQL Installer](https://dev.mysql.com/downloads/installer/)
- 下载并安装 [CMake](https://cmake.org/download/)
- 下载并安装 [Visual Studio 2022](https://visualstudio.microsoft.com/)

### 步骤 2: 配置 MySQL

```bash
# 启动 MySQL
sudo systemctl start mysqld
sudo systemctl enable mysqld

# 安全初始化
sudo mysql_secure_installation

# 登录 MySQL
sudo mysql -u root -p

# 创建数据库和用户
CREATE DATABASE smartsched CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'smartsched'@'localhost' IDENTIFIED BY 'your_strong_password';
GRANT ALL PRIVILEGES ON smartsched.* TO 'smartsched'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

### 步骤 3: 编译项目

```bash
# 克隆代码
git clone https://github.com/your-org/smartsched.git
cd smartsched

# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DDB_HOST=localhost \
         -DDB_NAME=smartsched \
         -DDB_USER=smartsched \
         -DDB_PASSWORD=your_password

# 编译
cmake --build . --config Release -j$(nproc)

# 安装
sudo cmake --install .
```

### 步骤 4: 初始化数据库

```bash
# 自动建表 (服务端启动时自动执行)
./bin/smartsched-server --init-db

# 或手动执行 SQL
mysql -u smartsched -p smartsched < ../doc/database/init.sql
```

### 步骤 5: 配置服务

创建配置文件 `/etc/smartsched/server.json`:

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8888,
    "thread_pool_size": 8,
    "max_connections": 1024
  },
  "database": {
    "host": "localhost",
    "port": 3306,
    "name": "smartsched",
    "user": "smartsched",
    "password": "your_password",
    "pool_size": 20
  },
  "security": {
    "encryption_enabled": true,
    "aes_key": "32BytesLongSecretKeyHere!!",
    "aes_iv": "16BytesInitVect"
  },
  "logging": {
    "level": "info",
    "dir": "/var/log/smartsched",
    "max_size": 10485760,
    "max_files": 30
  },
  "business": {
    "queue_capacity": 2000,
    "max_wait_time": 120,
    "call_timeout": 300,
    "auto_reassign": true
  }
}
```

### 步骤 6: 启动服务

**Linux (systemd):**

创建服务文件 `/etc/systemd/system/smartsched-server.service`:

```ini
[Unit]
Description=SmartSched HIS Server
After=network.target mysql.service
Wants=mysql.service

[Service]
Type=simple
User=smartsched
Group=smartsched
WorkingDirectory=/opt/smartsched
ExecStart=/opt/smartsched/bin/smartsched-server --config /etc/smartsched/server.json
Restart=always
RestartSec=10
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
```

```bash
# 重新加载 systemd
sudo systemctl daemon-reload

# 启动服务
sudo systemctl start smartsched-server
sudo systemctl enable smartsched-server

# 查看状态
sudo systemctl status smartsched-server
```

**Windows:**

```powershell
# 注册服务
sc create SmartSched binPath= "C:\Program Files\SmartSched\smartsched-server.exe --config C:\ProgramData\SmartSched\server.json" start= auto

# 启动服务
sc start SmartSched
```

---

## 配置说明

### 服务端配置 (server.json)

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `server.host` | string | "0.0.0.0" | 监听地址 |
| `server.port` | int | 8888 | 监听端口 |
| `server.thread_pool_size` | int | 4 | 工作线程数 |
| `server.max_connections` | int | 1024 | 最大并发连接 |
| `database.pool_size` | int | 10 | 数据库连接池大小 |
| `security.encryption_enabled` | bool | true | 是否启用加密 |
| `logging.level` | string | "info" | 日志级别: trace/debug/info/warn/error |
| `business.queue_capacity` | int | 1000 | 队列容量上限 |

### 客户端配置 (client.json)

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `server_host` | string | "localhost" | 服务器地址 |
| `server_port` | int | 8888 | 服务器端口 |
| `auto_reconnect` | bool | true | 自动重连 |
| `refresh_interval` | int | 2 | 刷新间隔(秒) |
| `sound_enabled` | bool | true | 声音提示 |

---

## Docker部署

### 使用 Docker Compose (推荐)

创建 `docker-compose.yml`:

```yaml
version: '3.8'

services:
  mysql:
    image: mysql:8.0
    container_name: smartsched-mysql
    restart: always
    environment:
      MYSQL_ROOT_PASSWORD: root_password
      MYSQL_DATABASE: smartsched
      MYSQL_USER: smartsched
      MYSQL_PASSWORD: smartsched_password
    volumes:
      - mysql_data:/var/lib/mysql
      - ./init.sql:/docker-entrypoint-initdb.d/init.sql:ro
    ports:
      - "3306:3306"
    networks:
      - smartsched-net
    command: --default-authentication-plugin=mysql_native_password

  server:
    image: smartsched/server:latest
    container_name: smartsched-server
    restart: always
    depends_on:
      - mysql
    ports:
      - "8888:8888"
    volumes:
      - ./server.json:/etc/smartsched/server.json:ro
      - server_logs:/var/log/smartsched
    networks:
      - smartsched-net
    environment:
      - TZ=Asia/Shanghai

  patient:
    image: smartsched/client:latest
    container_name: smartsched-patient
    restart: always
    depends_on:
      - server
    environment:
      - ROLE=patient
      - SERVER_HOST=server
    networks:
      - smartsched-net

  doctor:
    image: smartsched/client:latest
    container_name: smartsched-doctor
    restart: always
    depends_on:
      - server
    environment:
      - ROLE=doctor
      - SERVER_HOST=server
    networks:
      - smartsched-net

  admin:
    image: smartsched/admin:latest
    container_name: smartsched-admin
    restart: always
    depends_on:
      - server
    environment:
      - SERVER_HOST=server
    ports:
      - "8080:8080"
    networks:
      - smartsched-net

volumes:
  mysql_data:
  server_logs:

networks:
  smartsched-net:
    driver: bridge
```

启动服务:
```bash
docker-compose up -d
docker-compose ps
```

### 构建 Docker 镜像

```bash
# 构建服务端镜像
docker build -f Dockerfile.server -t smartsched/server:latest .

# 构建客户端镜像
docker build -f Dockerfile.client -t smartsched/client:latest .
```

---

## 集群部署

### 架构图

```
                    ┌─────────────────┐
                    │   Nginx/LB      │
                    │   (负载均衡)     │
                    └────────┬────────┘
                             │
           ┌─────────────────┼─────────────────┐
           │                 │                 │
    ┌──────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
    │ Server-01   │   │ Server-02   │   │ Server-03   │
    │ :8888       │   │ :8888       │   │ :8888       │
    └──────┬──────┘   └──────┬──────┘   └──────┬──────┘
           │                 │                 │
           └─────────────────┼─────────────────┘
                             │
                    ┌────────▼────────┐
                    │   MySQL Primary │
                    │   + Replicas    │
                    └─────────────────┘
```

### Nginx 负载均衡配置

```nginx
upstream smartsched_backend {
    least_conn;  # 最少连接优先
    server 192.168.1.101:8888 weight=5;
    server 192.168.1.102:8888 weight=5;
    server 192.168.1.103:8888 weight=5;
}

server {
    listen 8888;
    proxy_pass smartsched_backend;
    proxy_connect_timeout 5s;
    proxy_read_timeout 30s;
    
    # 健康检查
    health_check interval=5 falls=2 rises=3;
}
```

### MySQL 主从复制

**主库配置 (my.cnf):**
```ini
[mysqld]
server-id=1
log-bin=mysql-bin
binlog-format=ROW
sync-binlog=1
```

**从库配置:**
```ini
[mysqld]
server-id=2
relay-log=relay-bin
read-only=1
```

**配置复制:**
```sql
-- 主库创建复制用户
CREATE USER 'repl'@'%' IDENTIFIED BY 'repl_password';
GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';

-- 从库执行
CHANGE MASTER TO
    MASTER_HOST='master_ip',
    MASTER_USER='repl',
    MASTER_PASSWORD='repl_password',
    MASTER_LOG_FILE='mysql-bin.000001',
    MASTER_LOG_POS=xxx;
START SLAVE;
```

---

## 运维监控

### 日志查看

```bash
# 实时查看日志
tail -f /var/log/smartsched/server.log

# 查看错误日志
grep -i error /var/log/smartsched/server.log

# 日志轮转
logrotate -f /etc/logrotate.d/smartsched
```

### 性能监控

```bash
# 查看服务状态
systemctl status smartsched-server

# 查看进程资源使用
ps aux | grep smartsched-server
top -p $(pgrep smartsched-server)

# 查看连接数
ss -s
netstat -an | grep 8888 | wc -l

# 查看数据库连接
mysql -u smartsched -p -e "SHOW PROCESSLIST;"
```

### 备份策略

```bash
#!/bin/bash
# backup.sh - 每日备份脚本

BACKUP_DIR="/backup/smartsched"
DATE=$(date +%Y%m%d)
MYSQL_USER="smartsched"
MYSQL_PASSWORD="password"

# 创建备份目录
mkdir -p $BACKUP_DIR

# 备份数据库
mysqldump -u$MYSQL_USER -p$MYSQL_PASSWORD \
    --single-transaction \
    --routines \
    --triggers \
    smartsched | gzip > $BACKUP_DIR/smartsched_$DATE.sql.gz

# 保留最近30天备份
find $BACKUP_DIR -name "*.sql.gz" -mtime +30 -delete

echo "Backup completed: $BACKUP_DIR/smartsched_$DATE.sql.gz"
```

### Prometheus + Grafana 监控

配置 Prometheus 抓取指标:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'smartsched'
    static_configs:
      - targets: ['localhost:8889']  # 监控端口
    metrics_path: '/metrics'
```

---

## 故障排查

### 常见问题

**1. 服务启动失败**
```bash
# 检查端口占用
lsof -i:8888

# 检查配置文件语法
./bin/smartsched-server --validate-config

# 查看详细日志
./bin/smartsched-server --log-level=debug
```

**2. 数据库连接失败**
```bash
# 测试数据库连接
mysql -u smartsched -p -h localhost smartsched

# 检查连接池状态
./bin/smartsched-server --db-status
```

**3. 客户端连接超时**
```bash
# 检查服务端是否在监听
netstat -tlnp | grep 8888

# 检查防火墙规则
sudo iptables -L -n | grep 8888

# 查看连接数
ss -ant | grep 8888 | wc -l
```

**4. 性能下降**
```bash
# 查看慢查询
mysql -u root -p -e "SHOW FULL PROCESSLIST;"

# 查看内存使用
free -h
cat /proc/meminfo

# 查看IO状态
iostat -x 1
```

### 紧急恢复

```bash
# 停止服务
systemctl stop smartsched-server

# 从备份恢复数据库
gunzip < backup_20240101.sql.gz | mysql -u root -p smartsched

# 重启服务
systemctl start smartsched-server
```

---

## 安全加固

### 网络安全

```bash
# 配置防火墙 (ufw)
sudo ufw default deny incoming
sudo ufw allow 8888/tcp    # 服务端口
sudo ufw allow 8889/tcp    # 监控端口
sudo ufw enable

# 使用 SSL/TLS 终止 (Nginx)
# 见集群部署章节
```

### 数据库安全

```sql
-- 限制用户权限
REVOKE ALL PRIVILEGES ON smartsched.* FROM 'smartsched'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE ON smartsched.* TO 'smartsched'@'localhost';

-- 启用审计日志
SET GLOBAL general_log = 'ON';
SET GLOBAL general_log_file = '/var/log/mysql/audit.log';
```

### 应用安全

```json
{
  "security": {
    "encryption_enabled": true,
    "token_expiry_seconds": 3600,
    "max_login_attempts": 5,
    "password_min_length": 8,
    "enable_ip_whitelist": true,
    "allowed_ips": ["10.0.0.0/8", "192.168.0.0/16"]
  }
}
```

### 定期安全检查

```bash
#!/bin/bash
# security_check.sh

echo "=== 安全检查 ==="

# 检查未授权登录
grep "Failed password" /var/log/auth.log | tail -20

# 检查开放端口
ss -tulpn

# 检查文件完整性
sha256sum /opt/smartsched/bin/smartsched-server

# 检查数据库用户权限
mysql -u root -p -e "SELECT user, host FROM mysql.user;"
```

---

## 联系方式

- 技术支持: support@smartsched.example.com
- 紧急电话: 400-XXX-XXXX
- 文档: https://docs.smartsched.example.com

---

*最后更新: 2024-01-15*
*版本: 1.0.0*
