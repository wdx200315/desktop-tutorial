#include "HealthDashboardWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QTimer>
#include <QDateTime>

HealthDashboardWidget::HealthDashboardWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // 标题栏
    auto *headerLayout = new QHBoxLayout;
    QLabel *title = new QLabel("🔧 系统健康度看板");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    QPushButton *btnRefresh = new QPushButton("🔄 刷新");
    btnRefresh->setStyleSheet("QPushButton { padding: 8px 16px; border-radius: 4px; background: #4A90D9; color: white; border: none; }");
    connect(btnRefresh, &QPushButton::clicked, this, &HealthDashboardWidget::onRefresh);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(btnRefresh);
    mainLayout->addLayout(headerLayout);

    // 总体健康度
    auto *overallGroup = new QGroupBox("总体健康度");
    auto *overallLayout = new QHBoxLayout(overallGroup);
    lblOverallHealth = new QLabel("98%");
    lblOverallHealth->setStyleSheet("font-size: 48px; font-weight: bold; color: #4CAF50;");
    overallLayout->addWidget(lblOverallHealth, 0, Qt::AlignCenter);

    QVBoxLayout *statusLayout = new QVBoxLayout;
    QLabel *statusText = new QLabel("系统运行正常");
    statusText->setStyleSheet("font-size: 16px; color: #4CAF50;");
    lblLastCheck = new QLabel("最后检查: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    lblLastCheck->setStyleSheet("font-size: 12px; color: #666;");
    statusLayout->addWidget(statusText);
    statusLayout->addWidget(lblLastCheck);
    overallLayout->addLayout(statusLayout);
    overallLayout->addStretch();
    mainLayout->addWidget(overallGroup);

    // 分项健康度
    auto *detailGroup = new QGroupBox("分项健康度");
    auto *grid = new QGridLayout(detailGroup);

    // 充电桩健康
    auto *chargerBox = new QVBoxLayout;
    lblChargerHealth = new QLabel("95%");
    lblChargerHealth->setStyleSheet("font-size: 32px; font-weight: bold; color: #4CAF50;");
    chargerBox->addWidget(new QLabel("⚡ 充电桩"), 0, Qt::AlignCenter);
    chargerBox->addWidget(lblChargerHealth, 0, Qt::AlignCenter);
    chargerBox->addWidget(new QLabel("正常"), 0, Qt::AlignCenter);
    grid->addLayout(chargerBox, 0, 0);

    // 网络健康
    auto *networkBox = new QVBoxLayout;
    lblNetworkHealth = new QLabel("99%");
    lblNetworkHealth->setStyleSheet("font-size: 32px; font-weight: bold; color: #4CAF50;");
    networkBox->addWidget(new QLabel("🌐 网络连接"), 0, Qt::AlignCenter);
    networkBox->addWidget(lblNetworkHealth, 0, Qt::AlignCenter);
    networkBox->addWidget(new QLabel("正常"), 0, Qt::AlignCenter);
    grid->addLayout(networkBox, 0, 1);

    // 数据库健康
    auto *dbBox = new QVBoxLayout;
    lblDatabaseHealth = new QLabel("100%");
    lblDatabaseHealth->setStyleSheet("font-size: 32px; font-weight: bold; color: #4CAF50;");
    dbBox->addWidget(new QLabel("💾 数据库"), 0, Qt::AlignCenter);
    dbBox->addWidget(lblDatabaseHealth, 0, Qt::AlignCenter);
    dbBox->addWidget(new QLabel("正常"), 0, Qt::AlignCenter);
    grid->addLayout(dbBox, 0, 2);

    // 告警统计
    auto *alarmBox = new QVBoxLayout;
    QLabel *lblAlarms = new QLabel("3");
    lblAlarms->setStyleSheet("font-size: 32px; font-weight: bold; color: #FF9800;");
    alarmBox->addWidget(new QLabel("⚠️ 待处理告警"), 0, Qt::AlignCenter);
    alarmBox->addWidget(lblAlarms, 0, Qt::AlignCenter);
    alarmBox->addWidget(new QLabel("需关注"), 0, Qt::AlignCenter);
    grid->addLayout(alarmBox, 1, 0);

    // 在线设备
    auto *onlineBox = new QVBoxLayout;
    QLabel *lblOnline = new QLabel("45/50");
    lblOnline->setStyleSheet("font-size: 32px; font-weight: bold; color: #4A90D9;");
    onlineBox->addWidget(new QLabel("📱 在线设备"), 0, Qt::AlignCenter);
    onlineBox->addWidget(lblOnline, 0, Qt::AlignCenter);
    onlineBox->addWidget(new QLabel("90% 在线"), 0, Qt::AlignCenter);
    grid->addLayout(onlineBox, 1, 1);

    // 服务响应时间
    auto *responseBox = new QVBoxLayout;
    QLabel *lblResponse = new QLabel("45ms");
    lblResponse->setStyleSheet("font-size: 32px; font-weight: bold; color: #4CAF50;");
    responseBox->addWidget(new QLabel("⚡ 服务响应"), 0, Qt::AlignCenter);
    responseBox->addWidget(lblResponse, 0, Qt::AlignCenter);
    responseBox->addWidget(new QLabel("正常"), 0, Qt::AlignCenter);
    grid->addLayout(responseBox, 1, 2);

    mainLayout->addWidget(detailGroup);
    mainLayout->addStretch();

    // 定时刷新 (每30秒)
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &HealthDashboardWidget::updateStats);
    timer->start(30000);

    loadHealthData();
}

void HealthDashboardWidget::loadHealthData()
{
    // 模拟健康数据
    lblOverallHealth->setText("98%");
    lblChargerHealth->setText("95%");
    lblNetworkHealth->setText("99%");
    lblDatabaseHealth->setText("100%");
    lblLastCheck->setText("最后检查: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
}

void HealthDashboardWidget::onRefresh()
{
    loadHealthData();
}

void HealthDashboardWidget::updateStats()
{
    // 模拟数据轻微波动
    QStringList values = {"94%", "96%", "95%", "97%", "95%"};
    lblChargerHealth->setText(values[qrand() % values.size()]);
    lblLastCheck->setText("最后检查: " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
}
