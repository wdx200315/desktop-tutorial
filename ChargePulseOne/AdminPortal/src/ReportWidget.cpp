#include "common.h"
#include "ReportWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>

ReportWidget::ReportWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    auto *dateLayout = new QHBoxLayout;
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateLayout->addWidget(new QLabel("日期:"));
    dateLayout->addWidget(dateEdit);
    auto *btnDaily = new QPushButton("日报");
    connect(btnDaily, &QPushButton::clicked, this, &ReportWidget::onDaily);
    dateLayout->addWidget(btnDaily);
    auto *btnMonthly = new QPushButton("月报");
    connect(btnMonthly, &QPushButton::clicked, this, &ReportWidget::onMonthly);
    dateLayout->addWidget(btnMonthly);
    dateLayout->addStretch();
    layout->addLayout(dateLayout);

    auto *group = new QGroupBox("统计结果");
    auto *grid = new QGridLayout(group);
    lblOrders = new QLabel("订单: 0");
    lblEnergy = new QLabel("电量: 0 kWh");
    lblRevenue = new QLabel("收入: ¥0.00");
    lblAlarms = new QLabel("告警: 0");
    grid->addWidget(lblOrders, 0, 0);
    grid->addWidget(lblEnergy, 0, 1);
    grid->addWidget(lblRevenue, 1, 0);
    grid->addWidget(lblAlarms, 1, 1);
    layout->addWidget(group);

    connect(m_net, &NetworkManager::responseReceived, this, &ReportWidget::onResponse);
    onDaily(); // 默认加载当天
}

void ReportWidget::onDaily()
{
    QString date = dateEdit->date().toString("yyyy-MM-dd");
    m_net->getDailyReport(date);
}

void ReportWidget::onMonthly()
{
    QString month = dateEdit->date().toString("yyyy-MM");
    m_net->getMonthlyReport(month);
}

void ReportWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::REPORT_DAILY ||
        resp["cmd"].toString() == CMD::REPORT_MONTHLY) {
        if (resp["status"].toString() == "ok") {
            auto data = resp["data"].toObject();
            lblOrders->setText("订单: " + data["total_orders"].toString());
            lblEnergy->setText("电量: " + data["total_energy"].toString() + " kWh");
            lblRevenue->setText("收入: ¥" + data["total_revenue"].toString());
            lblAlarms->setText("告警: " + data["total_alarms"].toString());
        }
    }
}
