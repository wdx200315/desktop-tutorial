#include <QJsonArray>
#include <QJsonArray>
#include "common.h"
#include "OrderManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

OrderManagementWidget::OrderManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("状态:"));
    comboStatus = new QComboBox;
    comboStatus->addItems({"全部", "charging", "completed", "cancelled"});
    searchLayout->addWidget(comboStatus);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &OrderManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    auto *btnExport = new QPushButton("导出CSV");
    connect(btnExport, &QPushButton::clicked, this, &OrderManagementWidget::onExport);
    searchLayout->addWidget(btnExport);
    layout->addLayout(searchLayout);

    table = new QTableWidget;
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID", "用户ID", "充电桩", "模式", "开始时间", "状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    connect(m_net, &NetworkManager::responseReceived, this, &OrderManagementWidget::onResponse);
    loadOrders();
}

void OrderManagementWidget::loadOrders()
{
    QString status = comboStatus->currentText();
    if (status == "全部") status = "";
    m_net->getOrderList(1, 50, status);
}

void OrderManagementWidget::onSearch() { loadOrders(); }

void OrderManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::ORDER_LIST && resp["status"].toString() == "ok") {
        auto list = resp["data"].toObject()["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["user_id"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["charger_id"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["mode"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["start_time"].toString()));
            table->setItem(i, 5, new QTableWidgetItem(obj["status"].toString()));
        }
    }
}

void OrderManagementWidget::onExport()
{
    QString path = QFileDialog::getSaveFileName(this, "导出订单", "orders.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法写入文件");
        return;
    }
    QTextStream out(&file);
    // 写入表头
    for (int col = 0; col < table->columnCount(); ++col) {
        out << table->horizontalHeaderItem(col)->text() << (col < table->columnCount()-1 ? "," : "\n");
    }
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            out << table->item(row, col)->text() << (col < table->columnCount()-1 ? "," : "\n");
        }
    }
    file.close();
    QMessageBox::information(this, "导出", "导出成功");
}
