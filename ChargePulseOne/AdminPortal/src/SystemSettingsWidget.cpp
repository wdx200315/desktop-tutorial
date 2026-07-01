#include <QJsonArray>
#include "common.h"
#include "SystemSettingsWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

SystemSettingsWidget::SystemSettingsWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *layout = new QVBoxLayout(this);

    table = new QTableWidget;
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Key", "Value"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    auto *btnLayout = new QHBoxLayout;
    auto *btnAdd = new QPushButton("添加/修改");
    auto *btnDelete = new QPushButton("删除");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(btnAdd, &QPushButton::clicked, this, &SystemSettingsWidget::onAdd);
    connect(btnDelete, &QPushButton::clicked, this, &SystemSettingsWidget::onDelete);
    connect(m_net, &NetworkManager::responseReceived, this, &SystemSettingsWidget::onResponse);

    loadConfig();
}

void SystemSettingsWidget::loadConfig() { m_net->getSystemConfig(); }

void SystemSettingsWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::SYS_CONFIG_GET && resp["status"].toString() == "ok") {
        auto data = resp["data"].toObject();
        table->setRowCount(data.size());
        int i = 0;
        for (auto it = data.begin(); it != data.end(); ++it, ++i) {
            table->setItem(i, 0, new QTableWidgetItem(it.key()));
            table->setItem(i, 1, new QTableWidgetItem(it.value().toString()));
        }
    }
}

void SystemSettingsWidget::onAdd()
{
    bool ok;
    QString key = QInputDialog::getText(this, "添加配置", "Key:", QLineEdit::Normal, "", &ok);
    if (!ok || key.isEmpty()) return;
    QString value = QInputDialog::getText(this, "添加配置", "Value:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    m_net->setSystemConfig(key, value);
    loadConfig();
}

void SystemSettingsWidget::onDelete()
{
    int row = table->currentRow();
    if (row < 0) return;
    QString key = table->item(row, 0)->text();
    if (QMessageBox::question(this, "确认", "删除配置 " + key + " ?") == QMessageBox::Yes) {
        m_net->setSystemConfig(key, ""); // 服务端可能用空值删除，简化
        loadConfig();
    }
}
