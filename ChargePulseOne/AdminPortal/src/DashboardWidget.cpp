#include "DashboardWidget.h"
#include "NetworkManager.h"
#include "common.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDate>
#include <QFont>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

DashboardWidget::DashboardWidget(NetworkManager* net, QWidget* parent)
    : QWidget(parent), m_net(net)
{
    setupUI();

    // 初始化最近7天数据
    m_weeklyRevenue.resize(7);
    m_weeklyEnergy.resize(7);
    m_weeklyOrders.resize(7);
    for (int i = 0; i < 7; ++i) {
        m_weeklyRevenue[i] = 0;
        m_weeklyEnergy[i] = 0;
        m_weeklyOrders[i] = 0;
    }

    // 定时刷新 (每30秒)
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DashboardWidget::refreshStats);
    m_refreshTimer->start(30000);

    // 时钟更新定时器 (每秒)
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &DashboardWidget::updateClock);
    m_clockTimer->start(1000);

    connect(m_net, &NetworkManager::responseReceived, this, &DashboardWidget::onResponse);

    refresh();
}

DashboardWidget::~DashboardWidget() {
    if (m_refreshTimer) m_refreshTimer->stop();
    if (m_clockTimer) m_clockTimer->stop();
}

void DashboardWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题栏
    auto* titleLayout = new QHBoxLayout();
    QLabel* title = new QLabel("📊 运营概览");
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #333;");
    m_lblTime = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    m_lblTime->setStyleSheet("font-size: 14px; color: #666;");
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    titleLayout->addWidget(title);
    titleLayout->addWidget(m_lblTime);
    mainLayout->addLayout(titleLayout);

    // 第一行：核心指标卡片
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(12);

    // 今日订单
    auto* card1 = createStatCard("今日订单", "0", "单", "#4A90D9");
    m_lblTodayOrders = card1->findChild<QLabel*>("valueLabel");
    row1->addWidget(card1);

    // 今日收入
    auto* card2 = createStatCard("今日收入", "¥0.00", "", "#4CAF50");
    m_lblTodayRevenue = card2->findChild<QLabel*>("valueLabel");
    row1->addWidget(card2);

    // 累计充电量
    auto* card3 = createStatCard("累计充电量", "0.0", "度", "#FF9800");
    m_lblTotalEnergy = card3->findChild<QLabel*>("valueLabel");
    row1->addWidget(card3);

    mainLayout->addLayout(row1);

    // 第二行：设备状态
    auto* row2 = new QHBoxLayout();
    row2->setSpacing(12);

    // 充电桩状态
    auto* chargerCard = createInfoCard("充电桩状态");
    auto* chargerLayout = new QVBoxLayout();
    chargerLayout->setSpacing(8);

    QLabel* lblOnline = new QLabel("🟢 在线: 0 台");
    QLabel* lblCharging = new QLabel("⚡ 充电中: 0 台");
    QLabel* lblOffline = new QLabel("⚫ 离线: 0 台");
    QLabel* lblFault = new QLabel("🔴 故障: 0 台");

    lblOnline->setStyleSheet("font-size: 14px; color: #333;");
    lblCharging->setStyleSheet("font-size: 14px; color: #333;");
    lblOffline->setStyleSheet("font-size: 14px; color: #333;");
    lblFault->setStyleSheet("font-size: 14px; color: #333;");

    chargerLayout->addWidget(lblOnline);
    chargerLayout->addWidget(lblCharging);
    chargerLayout->addWidget(lblOffline);
    chargerLayout->addWidget(lblFault);
    chargerLayout->addStretch();

    m_lblOnlineChargers = lblOnline;
    m_lblChargingChargers = lblCharging;
    m_lblOfflineChargers = lblOffline;
    m_lblFaultChargers = lblFault;

    chargerCard->layout()->addItem(chargerLayout);
    row2->addWidget(chargerCard);

    // 告警信息
    auto* alarmCard = createInfoCard("最近告警");
    auto* alarmLayout = new QVBoxLayout();
    alarmLayout->setSpacing(8);

    QLabel* lblNoAlarm = new QLabel("暂无告警信息");
    lblNoAlarm->setStyleSheet("font-size: 14px; color: #4CAF50;");
    m_lblRecentAlarm = lblNoAlarm;

    alarmLayout->addWidget(lblNoAlarm);
    alarmLayout->addStretch();

    alarmCard->layout()->addItem(alarmLayout);
    row2->addWidget(alarmCard);

    mainLayout->addLayout(row2);

    // 第三行：快捷操作
    auto* row3 = new QHBoxLayout();
    row3->setSpacing(12);

    QPushButton* btnRefresh = new QPushButton("🔄 刷新数据");
    btnRefresh->setStyleSheet("QPushButton { padding: 10px 20px; border-radius: 6px; background: #4A90D9; color: white; border: none; } QPushButton:hover { background: #3A7BC8; }");
    connect(btnRefresh, &QPushButton::clicked, this, &DashboardWidget::refresh);

    QPushButton* btnViewOrders = new QPushButton("📋 查看订单");
    btnViewOrders->setStyleSheet("QPushButton { padding: 10px 20px; border-radius: 6px; background: #4CAF50; color: white; border: none; } QPushButton:hover { background: #3D8B40; }");
    connect(btnViewOrders, &QPushButton::clicked, this, []() {
        // 发送切换到订单管理页面的信号
    });

    row3->addWidget(btnRefresh);
    row3->addWidget(btnViewOrders);
    row3->addStretch();

    mainLayout->addLayout(row3);

    mainLayout->addStretch();
}

QWidget* DashboardWidget::createStatCard(const QString& title, const QString& value, const QString& unit, const QString& color) {
    auto* card = new QWidget();
    card->setStyleSheet(QString("QWidget { background: white; border-radius: 10px; padding: 15px; border: 1px solid #E0E0E0; }"));

    auto* layout = new QVBoxLayout(card);
    layout->setSpacing(4);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel* lblTitle = new QLabel(title);
    lblTitle->setStyleSheet("font-size: 13px; color: #666;");
    lblTitle->setObjectName("titleLabel");

    QLabel* lblValue = new QLabel(value);
    lblValue->setStyleSheet(QString("font-size: 26px; font-weight: bold; color: %1;").arg(color));
    lblValue->setObjectName("valueLabel");

    QLabel* lblUnit = new QLabel(unit);
    lblUnit->setStyleSheet("font-size: 12px; color: #999;");
    lblUnit->setObjectName("unitLabel");

    layout->addWidget(lblTitle);
    layout->addWidget(lblValue);
    layout->addWidget(lblUnit);
    layout->addStretch();

    return card;
}

QWidget* DashboardWidget::createInfoCard(const QString& title) {
    auto* card = new QWidget();
    card->setStyleSheet("QWidget { background: white; border-radius: 10px; padding: 15px; border: 1px solid #E0E0E0; }");

    auto* layout = new QVBoxLayout(card);
    layout->setSpacing(8);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel* lblTitle = new QLabel(title);
    lblTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #333; margin-bottom: 5px;");

    layout->addWidget(lblTitle);

    return card;
}

void DashboardWidget::refresh() {
    // 请求统计数据
    QJsonObject req;
    req["cmd"] = CMD::ADMIN_GET_DASHBOARD_STATS;
    m_net->sendRequest(req);

    // 请求充电桩状态
    QJsonObject chargerReq;
    chargerReq["cmd"] = CMD::ADMIN_GET_CHARGER_STATUS;
    m_net->sendRequest(chargerReq);

    // 更新模拟数据用于演示
    updateSimulatedData();
}

void DashboardWidget::updateSimulatedData() {
    // 模拟数据
    m_todayOrders = 128;
    m_todayRevenue = 2560.50;
    m_totalEnergy = 15680.5;
    m_onlineChargers = 45;
    m_chargingChargers = 12;
    m_totalChargers = 50;
    m_activeAlarms = 2;

    // 更新UI
    if (m_lblTodayOrders) {
        m_lblTodayOrders->setText(QString::number(m_todayOrders));
    }
    if (m_lblTodayRevenue) {
        m_lblTodayRevenue->setText(QString("¥%1").arg(m_todayRevenue, 0, 'f', 2));
    }
    if (m_lblTotalEnergy) {
        m_lblTotalEnergy->setText(QString::number(m_totalEnergy, 'f', 1));
    }
    if (m_lblOnlineChargers) {
        m_lblOnlineChargers->setText(QString("🟢 在线: %1 台").arg(m_onlineChargers));
    }
    if (m_lblChargingChargers) {
        m_lblChargingChargers->setText(QString("⚡ 充电中: %1 台").arg(m_chargingChargers));
    }
    if (m_lblOfflineChargers) {
        int offline = m_totalChargers - m_onlineChargers;
        m_lblOfflineChargers->setText(QString("⚫ 离线: %1 台").arg(offline));
    }
    if (m_lblFaultChargers) {
        m_lblFaultChargers->setText(QString("🔴 故障: %1 台").arg(m_activeAlarms));
    }
    if (m_lblRecentAlarm) {
        if (m_activeAlarms > 0) {
            m_lblRecentAlarm->setText(QString("⚠️ 有 %1 条未处理告警").arg(m_activeAlarms));
            m_lblRecentAlarm->setStyleSheet("font-size: 14px; color: #F44336;");
        } else {
            m_lblRecentAlarm->setText("✓ 暂无告警信息");
            m_lblRecentAlarm->setStyleSheet("font-size: 14px; color: #4CAF50;");
        }
    }
}

void DashboardWidget::refreshStats() {
    refresh();
}

void DashboardWidget::updateClock() {
    if (m_lblTime) {
        m_lblTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    }
}

void DashboardWidget::onResponse(const QJsonObject& resp) {
    QString cmd = resp["cmd"].toString();
    if (resp["status"] == "ok") {
        QJsonObject data = resp["data"].toObject();

        if (cmd == CMD::ADMIN_GET_DASHBOARD_STATS) {
            // 更新统计数据
            if (data.contains("today_orders")) {
                m_todayOrders = data["today_orders"].toInt();
                if (m_lblTodayOrders) {
                    m_lblTodayOrders->setText(QString::number(m_todayOrders));
                }
            }
            if (data.contains("today_revenue")) {
                m_todayRevenue = data["today_revenue"].toDouble();
                if (m_lblTodayRevenue) {
                    m_lblTodayRevenue->setText(QString("¥%1").arg(m_todayRevenue, 0, 'f', 2));
                }
            }
            if (data.contains("total_energy")) {
                m_totalEnergy = data["total_energy"].toDouble();
                if (m_lblTotalEnergy) {
                    m_lblTotalEnergy->setText(QString::number(m_totalEnergy, 'f', 1));
                }
            }
        } else if (cmd == CMD::ADMIN_GET_CHARGER_STATUS) {
            // 更新充电桩状态
            if (data.contains("online")) {
                m_onlineChargers = data["online"].toInt();
            }
            if (data.contains("charging")) {
                m_chargingChargers = data["charging"].toInt();
            }
            if (data.contains("total")) {
                m_totalChargers = data["total"].toInt();
            }
            if (data.contains("fault")) {
                m_activeAlarms = data["fault"].toInt();
            }

            if (m_lblOnlineChargers) {
                m_lblOnlineChargers->setText(QString("🟢 在线: %1 台").arg(m_onlineChargers));
            }
            if (m_lblChargingChargers) {
                m_lblChargingChargers->setText(QString("⚡ 充电中: %1 台").arg(m_chargingChargers));
            }
            if (m_lblOfflineChargers) {
                int offline = m_totalChargers - m_onlineChargers;
                m_lblOfflineChargers->setText(QString("⚫ 离线: %1 台").arg(offline));
            }
            if (m_lblFaultChargers) {
                m_lblFaultChargers->setText(QString("🔴 故障: %1 台").arg(m_activeAlarms));
            }
            if (m_lblRecentAlarm) {
                if (m_activeAlarms > 0) {
                    m_lblRecentAlarm->setText(QString("⚠️ 有 %1 条未处理告警").arg(m_activeAlarms));
                    m_lblRecentAlarm->setStyleSheet("font-size: 14px; color: #F44336;");
                } else {
                    m_lblRecentAlarm->setText("✓ 暂无告警信息");
                    m_lblRecentAlarm->setStyleSheet("font-size: 14px; color: #4CAF50;");
                }
            }
        }
    }
}
