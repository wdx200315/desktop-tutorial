#include "common.h"
#include "MemberManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QGroupBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QMessageBox>

MemberManagementWidget::MemberManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);

    // 统计卡片
    auto *statsGroup = new QGroupBox("会员统计");
    auto *statsGrid = new QGridLayout(statsGroup);
    lblTotalMembers = new QLabel("0");
    lblTotalMembers->setStyleSheet("font-size: 24px; font-weight: bold; color: #4A90D9;");
    lblActiveMembers = new QLabel("0");
    lblActiveMembers->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    lblNewThisMonth = new QLabel("0");
    lblNewThisMonth->setStyleSheet("font-size: 24px; font-weight: bold; color: #FF9800;");

    statsGrid->addWidget(new QLabel("总会员数"), 0, 0, Qt::AlignCenter);
    statsGrid->addWidget(lblTotalMembers, 1, 0, Qt::AlignCenter);
    statsGrid->addWidget(new QLabel("活跃会员"), 0, 1, Qt::AlignCenter);
    statsGrid->addWidget(lblActiveMembers, 1, 1, Qt::AlignCenter);
    statsGrid->addWidget(new QLabel("本月新增"), 0, 2, Qt::AlignCenter);
    statsGrid->addWidget(lblNewThisMonth, 1, 2, Qt::AlignCenter);
    mainLayout->addWidget(statsGroup);

    // 搜索栏
    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("会员等级:"));
    comboLevel = new QComboBox;
    comboLevel->addItems({"全部", "普通会员", "黄金会员", "铂金会员", "钻石会员"});
    searchLayout->addWidget(comboLevel);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &MemberManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    auto *btnRefresh = new QPushButton("刷新");
    connect(btnRefresh, &QPushButton::clicked, this, &MemberManagementWidget::onRefresh);
    searchLayout->addWidget(btnRefresh);
    searchLayout->addStretch();
    mainLayout->addLayout(searchLayout);

    // 会员列表
    table = new QTableWidget;
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "用户名", "手机", "等级", "积分", "注册时间", "状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);

    // 操作按钮
    auto *btnLayout = new QHBoxLayout;
    auto *btnAdjust = new QPushButton("调整等级");
    connect(btnAdjust, &QPushButton::clicked, this, &MemberManagementWidget::onAdjustLevel);
    btnLayout->addWidget(btnAdjust);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(m_net, &NetworkManager::responseReceived, this, &MemberManagementWidget::onResponse);

    // 模拟加载数据
    loadStats();
    loadMembers();
}

void MemberManagementWidget::loadStats()
{
    // 模拟统计数据
    lblTotalMembers->setText("1,280");
    lblActiveMembers->setText("856");
    lblNewThisMonth->setText("45");
}

void MemberManagementWidget::loadMembers()
{
    // 模拟会员数据（实际应从服务端获取）
    QString level = comboLevel->currentText();
    QStringList levels = {"全部", "普通会员", "黄金会员", "铂金会员", "钻石会员"};
    QStringList statuses = {"活跃", "活跃", "活跃", "冻结"};
    QStringList phones = {"138****1234", "139****5678", "136****9012", "137****3456"};

    int rows = 10;
    if (level != "全部") rows = 3;

    table->setRowCount(rows);
    for (int i = 0; i < rows; ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(1000 + i)));
        table->setItem(i, 1, new QTableWidgetItem("user_" + QString::number(1000 + i)));
        table->setItem(i, 2, new QTableWidgetItem(phones[i % phones.size()]));
        table->setItem(i, 3, new QTableWidgetItem(level == "全部" ? levels[i % levels.size()] : level));
        table->setItem(i, 4, new QTableWidgetItem(QString::number((i + 1) * 100)));
        table->setItem(i, 5, new QTableWidgetItem("2024-01-" + QString::number(10 + i).rightJustified(2, '0')));
        table->setItem(i, 6, new QTableWidgetItem(statuses[i % statuses.size()]));
    }
}

void MemberManagementWidget::onSearch()
{
    loadMembers();
}

void MemberManagementWidget::onRefresh()
{
    loadStats();
    loadMembers();
}

void MemberManagementWidget::onAdjustLevel()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要调整的会员");
        return;
    }
    QString username = table->item(row, 1)->text();
    QString currentLevel = table->item(row, 3)->text();

    bool ok;
    QStringList levels = {"普通会员", "黄金会员", "铂金会员", "钻石会员"};
    QString newLevel = QInputDialog::getItem(this, "调整会员等级",
                                            QString("为 %1 选择新等级").arg(username),
                                            levels, levels.indexOf(currentLevel), false, &ok);
    if (ok) {
        table->setItem(row, 3, new QTableWidgetItem(newLevel));
        QMessageBox::information(this, "成功", QString("已将 %1 调整为 %2").arg(username).arg(newLevel));
    }
}

void MemberManagementWidget::onResponse(const QJsonObject &resp)
{
    QString cmd = resp["cmd"].toString();
    if (resp["status"].toString() == "ok") {
        // 处理服务端响应
    }
}
