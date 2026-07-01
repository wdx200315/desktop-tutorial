#!/bin/bash
# =============================================================================
# SmartSched-HIS 一键部署脚本
# 医院门诊智慧调度系统
# =============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认配置
MODE="development"
DB_HOST="localhost"
DB_PORT="3306"
DB_NAME="smartsched"
DB_USER="smartsched"
DB_PASSWORD=""
SERVER_PORT=8888
INSTALL_DIR="/opt/smartsched"
LOG_DIR="/var/log/smartsched"

# 显示帮助
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --mode MODE              部署模式: development, production (默认: development)"
    echo "  --db-host HOST           数据库主机 (默认: localhost)"
    echo "  --db-port PORT           数据库端口 (默认: 3306)"
    echo "  --db-name NAME           数据库名称 (默认: smartsched)"
    echo "  --db-user USER           数据库用户 (默认: smartsched)"
    echo "  --db-password PASSWORD   数据库密码 (必需)"
    echo "  --server-port PORT       服务端端口 (默认: 8888)"
    echo "  --install-dir DIR        安装目录 (默认: /opt/smartsched)"
    echo "  --help                   显示帮助"
    echo ""
    echo "Examples:"
    echo "  $0 --mode production --db-password mysecretpass"
    echo "  $0 --db-host 192.168.1.100 --db-password pass123"
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --mode)
            MODE="$2"
            shift 2
            ;;
        --db-host)
            DB_HOST="$2"
            shift 2
            ;;
        --db-port)
            DB_PORT="$2"
            shift 2
            ;;
        --db-name)
            DB_NAME="$2"
            shift 2
            ;;
        --db-user)
            DB_USER="$2"
            shift 2
            ;;
        --db-password)
            DB_PASSWORD="$2"
            shift 2
            ;;
        --server-port)
            SERVER_PORT="$2"
            shift 2
            ;;
        --install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 检查必需参数
if [ -z "$DB_PASSWORD" ]; then
    echo -e "${RED}错误: --db-password 是必需参数${NC}"
    show_help
    exit 1
fi

# 显示配置
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  SmartSched-HIS 部署配置${NC}"
echo -e "${BLUE}========================================${NC}"
echo "  部署模式: $MODE"
echo "  数据库: $DB_HOST:$DB_PORT/$DB_NAME"
echo "  数据库用户: $DB_USER"
echo "  服务端口: $SERVER_PORT"
echo "  安装目录: $INSTALL_DIR"
echo -e "${BLUE}========================================${NC}"
echo ""

# 检查是否为root用户
if [ "$EUID" -ne 0 ] && [ "$MODE" = "production" ]; then
    echo -e "${RED}错误: 生产模式部署需要root权限${NC}"
    echo "请使用: sudo $0 ..."
    exit 1
fi

# 检查Docker是否可用
check_docker() {
    if command -v docker &> /dev/null && command -v docker-compose &> /dev/null; then
        return 0
    fi
    return 1
}

# 检查Docker模式
if check_docker; then
    echo -e "${GREEN}检测到Docker环境${NC}"
    deploy_docker
else
    echo -e "${YELLOW}未检测到Docker，使用原生部署${NC}"
    deploy_native
fi

# Docker部署
deploy_docker() {
    echo -e "${GREEN}[1/4] 创建配置文件...${NC}"
    
    # 创建临时目录
    TEMP_DIR=$(mktemp -d)
    cp -r docker/* "$TEMP_DIR/"
    
    # 生成配置文件
    cat > "$TEMP_DIR/server.json" << EOF
{
  "server": {
    "host": "0.0.0.0",
    "port": $SERVER_PORT,
    "thread_pool_size": 8,
    "max_connections": 1024
  },
  "database": {
    "host": "mysql",
    "port": $DB_PORT,
    "name": "$DB_NAME",
    "user": "$DB_USER",
    "password": "$DB_PASSWORD",
    "pool_size": 20
  },
  "security": {
    "encryption_enabled": true,
    "aes_key": "$(openssl rand -base64 24 | tr -d '/+=' | head -c 32)",
    "aes_iv": "$(openssl rand -base64 16 | tr -d '/+=' | head -c 16)"
  },
  "logging": {
    "level": "info",
    "dir": "/var/log/smartsched",
    "max_size": 10485760,
    "max_files": 30,
    "async": true
  },
  "business": {
    "queue_capacity": 2000,
    "max_wait_time": 120,
    "call_timeout": 300,
    "auto_reassign": true
  }
}
EOF

    echo -e "${GREEN}[2/4] 启动Docker服务...${NC}"
    cd "$TEMP_DIR"
    MYSQL_ROOT_PASSWORD="$DB_PASSWORD" MYSQL_DATABASE="$DB_NAME" MYSQL_USER="$DB_USER" MYSQL_PASSWORD="$DB_PASSWORD" docker-compose up -d
    
    echo -e "${GREEN}[3/4] 等待服务就绪...${NC}"
    sleep 10
    
    # 等待MySQL就绪
    for i in {1..30}; do
        if docker exec smartsched-mysql mysqladmin ping -h localhost --silent 2>/dev/null; then
            break
        fi
        echo -n "."
        sleep 2
    done
    echo ""
    
    echo -e "${GREEN}[4/4] 验证部署...${NC}"
    if curl -s http://localhost:$SERVER_PORT/health > /dev/null 2>&1; then
        echo -e "${GREEN}服务启动成功!${NC}"
    else
        echo -e "${YELLOW}服务可能还在启动中，请稍后检查${NC}"
    fi
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Docker部署完成!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo "  服务端口: $SERVER_PORT"
    echo "  管理后台: http://localhost:8080"
    echo "  监控面板: http://localhost:3000 (admin/admin)"
    echo ""
    echo "常用命令:"
    echo "  查看日志: docker-compose logs -f server"
    echo "  停止服务: docker-compose down"
    echo "  重启服务: docker-compose restart"
    echo ""
    
    rm -rf "$TEMP_DIR"
}

# 原生部署
deploy_native() {
    echo -e "${GREEN}[1/6] 检查依赖...${NC}"
    
    # 检查MySQL
    if ! command -v mysql &> /dev/null; then
        echo -e "${RED}错误: MySQL未安装${NC}"
        echo "请安装MySQL: sudo apt install mysql-server mysql-client"
        exit 1
    fi
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}错误: CMake未安装${NC}"
        echo "请安装CMake: sudo apt install cmake build-essential"
        exit 1
    fi
    
    # 检查OpenSSL
    if ! command -v openssl &> /dev/null; then
        echo -e "${RED}错误: OpenSSL未安装${NC}"
        echo "请安装OpenSSL: sudo apt install libssl-dev"
        exit 1
    fi
    
    echo -e "${GREEN}[2/6] 创建系统用户...${NC}"
    if ! id -u smartsched &> /dev/null; then
        sudo useradd -r -m -s /bin/bash smartsched
        echo "用户smartsched已创建"
    else
        echo "用户smartsched已存在"
    fi
    
    echo -e "${GREEN}[3/6] 配置MySQL数据库...${NC}"
    sudo mysql -e "CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
    sudo mysql -e "CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASSWORD';"
    sudo mysql -e "GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'localhost';"
    sudo mysql -e "FLUSH PRIVILEGES;"
    echo "数据库配置完成"
    
    echo -e "${GREEN}[4/6] 创建目录结构...${NC}"
    sudo mkdir -p "$INSTALL_DIR/bin"
    sudo mkdir -p "$INSTALL_DIR/lib"
    sudo mkdir -p "$INSTALL_DIR/config"
    sudo mkdir -p "$LOG_DIR"
    sudo chown -R smartsched:smartsched "$INSTALL_DIR" "$LOG_DIR"
    
    echo -e "${GREEN}[5/6] 编译项目...${NC}"
    cd "$(dirname "$0")"
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
             -DDB_HOST="$DB_HOST" \
             -DDB_NAME="$DB_NAME" \
             -DDB_USER="$DB_USER" \
             -DDB_PASSWORD="$DB_PASSWORD"
    cmake --build . --config Release -j$(nproc)
    
    echo -e "${GREEN}[6/6] 安装服务...${NC}"
    sudo cmake --install .
    
    # 创建配置文件
    sudo tee "$INSTALL_DIR/config/server.json" > /dev/null << EOF
{
  "server": {
    "host": "0.0.0.0",
    "port": $SERVER_PORT,
    "thread_pool_size": 8,
    "max_connections": 1024
  },
  "database": {
    "host": "$DB_HOST",
    "port": $DB_PORT,
    "name": "$DB_NAME",
    "user": "$DB_USER",
    "password": "$DB_PASSWORD",
    "pool_size": 20
  },
  "security": {
    "encryption_enabled": true,
    "aes_key": "$(openssl rand -base64 24 | tr -d '/+=' | head -c 32)",
    "aes_iv": "$(openssl rand -base64 16 | tr -d '/+=' | head -c 16)"
  },
  "logging": {
    "level": "info",
    "dir": "$LOG_DIR",
    "max_size": 10485760,
    "max_files": 30,
    "async": true
  },
  "business": {
    "queue_capacity": 2000,
    "max_wait_time": 120,
    "call_timeout": 300,
    "auto_reassign": true
  }
}
EOF
    sudo chown smartsched:smartsched "$INSTALL_DIR/config/server.json"
    
    # 配置systemd服务
    if [ -d /etc/systemd ]; then
        sudo tee /etc/systemd/system/smartsched-server.service > /dev/null << EOF
[Unit]
Description=SmartSched HIS Server
After=network.target mysql.service
Wants=mysql.service

[Service]
Type=simple
User=smartsched
Group=smartsched
WorkingDirectory=$INSTALL_DIR
ExecStart=$INSTALL_DIR/bin/smartsched-server --config $INSTALL_DIR/config/server.json
Restart=always
RestartSec=10
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
EOF
        sudo systemctl daemon-reload
        sudo systemctl enable smartsched-server
        sudo systemctl start smartsched-server
        echo "服务已配置为开机启动"
    fi
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  原生部署完成!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo "  安装目录: $INSTALL_DIR"
    echo "  服务端口: $SERVER_PORT"
    echo "  日志目录: $LOG_DIR"
    echo ""
    echo "常用命令:"
    echo "  启动服务: sudo systemctl start smartsched-server"
    echo "  停止服务: sudo systemctl stop smartsched-server"
    echo "  查看日志: sudo journalctl -u smartsched-server -f"
    echo "  查看状态: sudo systemctl status smartsched-server"
    echo ""
}

# 完成后提示
echo -e "${GREEN}部署完成!${NC}"
echo ""
echo "下一步:"
echo "  1. 启动客户端应用进行测试"
echo "  2. 访问管理后台配置科室和医生"
echo "  3. 使用压力测试工具验证系统性能:"
echo "     ./build/bin/stresstest -h localhost -p $SERVER_PORT -c 100 -r 100"
