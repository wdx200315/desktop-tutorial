#include "common.h"
#include <QJsonArray>
#include "common.h"
#include "LogAuditWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>

LogAuditWidget::LogAuditWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("类型:"));
    comboType = new QComboBox;
    comboType->addItems({"全部", "login", "logout", "charge", "config", "user"});
    searchLayout->addWidget(comboType);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &LogAuditWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    layout->addLayout(searchLayout);

    table = new QTableWidget;
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID", "用户ID", "类型", "描述", "时间"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    connect(m_net, &NetworkManager::responseReceived, this, &LogAuditWidget::onResponse);
    loadLogs();
}

void LogAuditWidget::loadLogs()
{
    QString type = comboType->currentText();
    if (type == "全部") type = "";
    m_net->getOperationLogs(1, 50, type);
}

void LogAuditWidget::onSearch() { loadLogs(); }

void LogAuditWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::LOG_QUERY && resp["status"].toString() == "ok") {
        auto list = resp["data"].toObject()["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["user_id"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["type"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["description"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["created_at"].toString()));
        }
    }
}
