#include "AdminMainWindow.h"
#include "NetworkManager.h"
#include "common.h"
#include "DashboardWidget.h"
#include "UserManagementWidget.h"
#include "ChargerManagementWidget.h"
#include "OrderManagementWidget.h"
#include "AlarmManagementWidget.h"
#include "RateManagementWidget.h"
#include "ReportWidget.h"
#include "SystemSettingsWidget.h"
#include "LogAuditWidget.h"
#include "ReservationManagementWidget.h"
#include "CouponManagementWidget.h"
#include "MemberManagementWidget.h"
#include "HealthDashboardWidget.h"
#include "FinanceReportWidget.h"
#include "PushNotificationWidget.h"

#include <QApplication>
#include <QHeaderView>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>

AdminMainWindow::AdminMainWindow(NetworkManager *net, QWidget *parent)
    : QMainWindow(parent), m_net(net)
{
    setWindowTitle("ChargePulse 管理后台");
    resize(1400, 900);

    // 创建堆叠窗口
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // 创建登录页面
    createLoginPage();

    // 创建主页面
    createMainPage();

    // 默认显示登录页
    m_stack->addWidget(m_loginWidget);
    m_stack->addWidget(m_mainWidget);
    m_stack->setCurrentWidget(m_loginWidget);

    // 连接信号
    connect(m_net, &NetworkManager::responseReceived, this, &AdminMainWindow::onResponse);
}

AdminMainWindow::~AdminMainWindow() {}

void AdminMainWindow::createLoginPage()
{
    m_loginWidget = new QWidget();
    m_loginWidget->setStyleSheet("background-color: #f5f5f5;");

    auto *mainLayout = new QHBoxLayout(m_loginWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 居中容器
    auto *container = new QWidget();
    container->setMaximumWidth(400);
    container->setMinimumWidth(350);

    auto *loginLayout = new QVBoxLayout(container);
    loginLayout->setContentsMargins(50, 80, 50, 50);
    loginLayout->setSpacing(20);

    // Logo 和标题
    QLabel *logo = new QLabel("⚡ ChargePulse");
    logo->setStyleSheet("font-size: 36px; font-weight: bold; color: #4A90D9;");
    logo->setAlignment(Qt::AlignCenter);

    QLabel *subtitle = new QLabel("管理后台");
    subtitle->setStyleSheet("font-size: 18px; color: #666;");
    subtitle->setAlignment(Qt::AlignCenter);

    // 登录表单
    auto *formWidget = new QWidget();
    formWidget->setStyleSheet("background: white; border-radius: 12px; padding: 30px;");
    auto *formLayout = new QVBoxLayout(formWidget);
    formLayout->setSpacing(16);

    QLabel *formTitle = new QLabel("管理员登录");
    formTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    formLayout->addWidget(formTitle);

    // 用户名
    QLabel *userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("font-size: 13px; color: #666;");
    formLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setText("admin");
    m_usernameEdit->setStyleSheet("QLineEdit { padding: 12px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; } QLineEdit:focus { border-color: #4A90D9; }");
    formLayout->addWidget(m_usernameEdit);

    // 密码
    QLabel *pwdLabel = new QLabel("密码");
    pwdLabel->setStyleSheet("font-size: 13px; color: #666;");
    formLayout->addWidget(pwdLabel);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setText("admin123");
    m_passwordEdit->setStyleSheet("QLineEdit { padding: 12px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; } QLineEdit:focus { border-color: #4A90D9; }");
    formLayout->addWidget(m_passwordEdit);

    // 状态标签
    m_loginStatus = new QLabel();
    m_loginStatus->setStyleSheet("color: #F44336; font-size: 13px;");
    m_loginStatus->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(m_loginStatus);

    // 登录按钮
    QPushButton *loginBtn = new QPushButton("登 录");
    loginBtn->setStyleSheet("QPushButton { background: #4A90D9; color: white; padding: 14px; border: none; border-radius: 6px; font-size: 16px; font-weight: bold; } QPushButton:hover { background: #3A7BC8; }");
    connect(loginBtn, &QPushButton::clicked, this, &AdminMainWindow::onLogin);
    formLayout->addWidget(loginBtn);

    // 提示信息
    QLabel *tip = new QLabel("提示: 默认账号 admin / admin123");
    tip->setStyleSheet("color: #999; font-size: 12px;");
    tip->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(tip);

    loginLayout->addWidget(logo);
    loginLayout->addWidget(subtitle);
    loginLayout->addSpacing(20);
    loginLayout->addWidget(formWidget);
    loginLayout->addStretch();

    mainLayout->addStretch();
    mainLayout->addWidget(container);
    mainLayout->addStretch();
}

void AdminMainWindow::createMainPage()
{
    m_mainWidget = new QWidget();

    auto *mainLayout = new QHBoxLayout(m_mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧导航
    auto *navWidget = new QWidget();
    navWidget->setFixedWidth(220);
    navWidget->setStyleSheet("background: #2c3e50;");
    auto *navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    // Logo
    QLabel *logo = new QLabel("⚡ ChargePulse");
    logo->setStyleSheet("color: white; font-size: 18px; font-weight: bold; padding: 20px; background: #1a252f;");
    logo->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(logo);

    // 用户信息
    QWidget *userWidget = new QWidget();
    userWidget->setStyleSheet("background: #34495e; padding: 15px;");
    auto *userLayout = new QHBoxLayout(userWidget);
    userLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *avatar = new QLabel("👤");
    avatar->setStyleSheet("font-size: 24px;");
    userLayout->addWidget(avatar);

    m_userLabel = new QLabel("管理员");
    m_userLabel->setStyleSheet("color: white; font-size: 14px;");
    userLayout->addWidget(m_userLabel);
    userLayout->addStretch();

    navLayout->addWidget(userWidget);

    // 导航列表
    m_navList = new QListWidget();
    m_navList->setStyleSheet(R"(
        QListWidget {
            background: #2c3e50;
            border: none;
            padding-top: 10px;
        }
        QListWidget::item {
            color: #bdc3c7;
            padding: 12px 20px;
            font-size: 14px;
            border: none;
        }
        QListWidget::item:hover {
            background: #34495e;
            color: white;
        }
        QListWidget::item:selected {
            background: #4A90D9;
            color: white;
        }
    )");

    QStringList navItems = {
        "📊 运营概览",
        "👥 用户管理",
        "⚡ 充电桩管理",
        "📋 订单管理",
        "⚠️ 告警管理",
        "💰 费率管理",
        "📈 统计报表",
        "💳 财务统计",
        "🏥 健康监控",
        "📦 预约管理",
        "🎫 优惠券管理",
        "👑 会员管理",
        "📢 消息推送",
        "⚙️ 系统设置",
        "📝 日志审计"
    };

    for (const QString &item : navItems) {
        m_navList->addItem(item);
    }

    m_navList->setCurrentRow(0);
    connect(m_navList, &QListWidget::currentRowChanged, this, &AdminMainWindow::onNavItemClicked);

    navLayout->addWidget(m_navList);

    // 底部操作
    QWidget *bottomWidget = new QWidget();
    bottomWidget->setStyleSheet("background: #1a252f; padding: 15px;");
    auto *bottomLayout = new QHBoxLayout(bottomWidget);

    QPushButton *refreshBtn = new QPushButton("🔄 刷新");
    refreshBtn->setStyleSheet("QPushButton { background: transparent; color: #bdc3c7; border: 1px solid #555; padding: 8px 15px; border-radius: 4px; } QPushButton:hover { background: #34495e; }");
    connect(refreshBtn, &QPushButton::clicked, this, &AdminMainWindow::onRefresh);
    bottomLayout->addWidget(refreshBtn);

    QPushButton *logoutBtn = new QPushButton("🚪 退出");
    logoutBtn->setStyleSheet("QPushButton { background: transparent; color: #e74c3c; border: 1px solid #e74c3c; padding: 8px 15px; border-radius: 4px; } QPushButton:hover { background: #e74c3c; color: white; }");
    connect(logoutBtn, &QPushButton::clicked, this, &AdminMainWindow::onLogout);
    bottomLayout->addWidget(logoutBtn);

    navLayout->addWidget(bottomWidget);

    mainLayout->addWidget(navWidget);

    // 右侧内容区
    m_stack = new QStackedWidget();
    mainLayout->addWidget(m_stack, 1);

    // 添加各个功能模块
    m_stack->addWidget(new DashboardWidget(m_net));          // 0
    m_stack->addWidget(new UserManagementWidget(m_net));      // 1
    m_stack->addWidget(new ChargerManagementWidget(m_net));   // 2
    m_stack->addWidget(new OrderManagementWidget(m_net));    // 3
    m_stack->addWidget(new AlarmManagementWidget(m_net));    // 4
    m_stack->addWidget(new RateManagementWidget(m_net));      // 5
    m_stack->addWidget(new ReportWidget(m_net));             // 6
    m_stack->addWidget(new FinanceReportWidget(m_net));       // 7
    m_stack->addWidget(new HealthDashboardWidget(m_net));    // 8
    m_stack->addWidget(new ReservationManagementWidget(m_net)); // 9
    m_stack->addWidget(new CouponManagementWidget(m_net));   // 10
    m_stack->addWidget(new MemberManagementWidget(m_net));   // 11
    m_stack->addWidget(new PushNotificationWidget(m_net));   // 12
    m_stack->addWidget(new SystemSettingsWidget(m_net));      // 13
    m_stack->addWidget(new LogAuditWidget(m_net));           // 14
}

void AdminMainWindow::onLogin()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_loginStatus->setText("请输入用户名和密码");
        return;
    }

    m_loginStatus->setText("正在登录...");

    // 发送登录请求
    QJsonObject req;
    req["cmd"] = CMD::ADMIN_LOGIN;
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;
    req["data"] = data;
    m_net->sendRequest(req);
}

void AdminMainWindow::onLogout()
{
    if (QMessageBox::question(this, "确认退出", "确定要退出登录吗？") == QMessageBox::Yes) {
        m_isLoggedIn = false;
        m_stack->setCurrentIndex(0);
    }
}

void AdminMainWindow::onRefresh()
{
    // 刷新当前页面
}

void AdminMainWindow::onNavItemClicked(int index)
{
    m_stack->setCurrentIndex(index);
}

void AdminMainWindow::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"] == CMD::ADMIN_LOGIN) {
        if (resp["status"] == "ok") {
            m_isLoggedIn = true;
            m_loginStatus->setText("");

            QJsonObject data = resp["data"].toObject();
            QString adminName = data["username"].toString("管理员");
            m_userLabel->setText(adminName);

            // 切换到主界面
            m_stack->setCurrentWidget(m_mainWidget);
            QMessageBox::information(this, "登录成功", QString("欢迎，%1！").arg(adminName));
        } else {
            m_loginStatus->setText("用户名或密码错误");
        }
    }
}
