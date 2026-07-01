#include <QJsonArray>
#include "common.h"
#include "ReservationManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

ReservationManagementWidget::ReservationManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    // 搜索栏
    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("状态:"));
    comboStatus = new QComboBox;
    comboStatus->addItems({"全部", "pending", "executed", "cancelled"});
    searchLayout->addWidget(comboStatus);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &ReservationManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    auto *btnCancel = new QPushButton("取消预约");
    connect(btnCancel, &QPushButton::clicked, this, &ReservationManagementWidget::onCancel);
    searchLayout->addWidget(btnCancel);
    searchLayout->addStretch();
    layout->addLayout(searchLayout);

    table = new QTableWidget;
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID", "用户ID", "充电桩", "预约时间", "模式", "状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    connect(m_net, &NetworkManager::responseReceived, this, &ReservationManagementWidget::onResponse);
    loadReservations();
}

void ReservationManagementWidget::loadReservations()
{
    // 调用服务端预约列表（需服务端支持管理端查看所有预约）
    // 目前服务端 ReservationService 只返回当前用户的预约，管理端需扩展
    // 临时方案：直接发 RESERVE 命令，action=list，不传 user_id 则返回全部
    QJsonObject req;
    req["cmd"] = CMD::RESERVE;
    QJsonObject data;
    data["action"] = "list";
    data["page"] = 1;
    data["size"] = 50;
    QString status = comboStatus->currentText();
    if (status != "全部") data["status"] = status;
    req["data"] = data;
    m_net->sendRequest(req);
}

void ReservationManagementWidget::onSearch() { loadReservations(); }

void ReservationManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::RESERVE && resp["status"].toString() == "ok") {
        auto list = resp["data"].toObject()["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["user_id"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["charger_id"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["reserve_time"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["mode"].toString()));
            table->setItem(i, 5, new QTableWidgetItem(obj["status"].toString()));
        }
    }
}

void ReservationManagementWidget::onCancel()
{
    int row = table->currentRow();
    if (row < 0) return;
    int id = table->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, "确认", "取消该预约？") == QMessageBox::Yes) {
        QJsonObject req;
        req["cmd"] = CMD::RESERVE;
        QJsonObject data;
        data["action"] = "cancel";
        data["id"] = id;
        req["data"] = data;
        m_net->sendRequest(req);
        loadReservations();
    }
}
