#include <QJsonArray>
#include "common.h"
#include "AlarmManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

AlarmManagementWidget::AlarmManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    table = new QTableWidget;
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID", "充电桩", "级别", "消息", "状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    auto *btnLayout = new QHBoxLayout;
    auto *btnHandle = new QPushButton("处理告警");
    auto *btnRefresh = new QPushButton("刷新");
    btnLayout->addWidget(btnHandle);
    btnLayout->addWidget(btnRefresh);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(btnHandle, &QPushButton::clicked, this, &AlarmManagementWidget::onHandle);
    connect(btnRefresh, &QPushButton::clicked, this, &AlarmManagementWidget::refresh);
    connect(m_net, &NetworkManager::responseReceived, this, &AlarmManagementWidget::onResponse);

    loadAlarms();
}

void AlarmManagementWidget::loadAlarms() { m_net->getAlarmList(1, 50); }
void AlarmManagementWidget::refresh() { loadAlarms(); }

void AlarmManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::ALARM_LIST && resp["status"].toString() == "ok") {
        auto list = resp["data"].toObject()["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["charger_id"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["level"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["message"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["status"].toString()));
        }
    }
}

void AlarmManagementWidget::onHandle()
{
    int row = table->currentRow();
    if (row < 0) return;
    int alarmId = table->item(row, 0)->text().toInt();
    bool ok;
    QString note = QInputDialog::getText(this, "处理告警", "处理备注:", QLineEdit::Normal, "", &ok);
    if (ok) {
        m_net->handleAlarm(alarmId, note);
        loadAlarms();
    }
}
