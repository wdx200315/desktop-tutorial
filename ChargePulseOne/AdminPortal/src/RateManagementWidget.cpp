#include <QJsonArray>
#include "common.h"
#include "RateManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

RateManagementWidget::RateManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    table = new QTableWidget;
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID", "名称", "电价(¥/kWh)", "服务费(¥)", "时段"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    auto *btnLayout = new QHBoxLayout;
    auto *btnAdd = new QPushButton("添加");
    auto *btnEdit = new QPushButton("编辑");
    auto *btnDelete = new QPushButton("删除");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(btnAdd, &QPushButton::clicked, this, &RateManagementWidget::onAdd);
    connect(btnEdit, &QPushButton::clicked, this, &RateManagementWidget::onEdit);
    connect(btnDelete, &QPushButton::clicked, this, &RateManagementWidget::onDelete);
    connect(m_net, &NetworkManager::responseReceived, this, &RateManagementWidget::onResponse);

    loadRates();
}

void RateManagementWidget::loadRates() { m_net->getRateList(); }

void RateManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::RATE_LIST && resp["status"].toString() == "ok") {
        auto list = resp["data"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["name"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["price_kwh"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["service_fee"].toString()));
            QString period = obj["start_time"].toString() + " - " + obj["end_time"].toString();
            table->setItem(i, 4, new QTableWidgetItem(period));
        }
    }
}

void RateManagementWidget::onAdd()
{
    bool ok;
    QString name = QInputDialog::getText(this, "添加费率", "名称:", QLineEdit::Normal, "标准费率", &ok);
    if (!ok) return;
    double price = QInputDialog::getDouble(this, "添加费率", "电价(¥/kWh):", 1.0, 0, 10, 2, &ok);
    if (!ok) return;
    double service = QInputDialog::getDouble(this, "添加费率", "服务费(¥):", 0.5, 0, 10, 2, &ok);
    if (!ok) return;
    QString start = QInputDialog::getText(this, "添加费率", "开始时间(HH:MM):", QLineEdit::Normal, "00:00", &ok);
    if (!ok) return;
    QString end = QInputDialog::getText(this, "添加费率", "结束时间(HH:MM):", QLineEdit::Normal, "23:59", &ok);
    if (!ok) return;

    QJsonObject data;
    data["name"] = name;
    data["price_kwh"] = price;
    data["service_fee"] = service;
    data["start_time"] = start;
    data["end_time"] = end;
    m_net->addRate(data);
    loadRates();
}

void RateManagementWidget::onEdit()
{
    int row = table->currentRow();
    if (row < 0) return;
    int id = table->item(row, 0)->text().toInt();
    // 简化编辑，实际可弹出完整表单
    double price = QInputDialog::getDouble(this, "编辑费率", "新电价:", table->item(row, 2)->text().toDouble(), 0, 10, 2);
    QJsonObject data;
    data["price_kwh"] = price;
    m_net->updateRate(id, data);
    loadRates();
}

void RateManagementWidget::onDelete()
{
    int row = table->currentRow();
    if (row < 0) return;
    int id = table->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, "确认", "删除该费率？") == QMessageBox::Yes) {
        m_net->deleteRate(id);
        loadRates();
    }
}
