#include <QJsonArray>
#include <QJsonArray>
#include "common.h"
#include "ChargerManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

ChargerManagementWidget::ChargerManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("关键词:"));
    editKeyword = new QLineEdit;
    editKeyword->setPlaceholderText("编号/位置");
    searchLayout->addWidget(editKeyword);
    searchLayout->addWidget(new QLabel("状态:"));
    comboStatus = new QComboBox;
    comboStatus->addItems({"全部", "online", "offline", "charging", "fault"});
    searchLayout->addWidget(comboStatus);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &ChargerManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    mainLayout->addLayout(searchLayout);

    table = new QTableWidget;
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "编号", "状态", "功率(kW)", "温度(°C)", "位置", "操作"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);

    auto *btnLayout = new QHBoxLayout;
    auto *btnStart = new QPushButton("启动");
    auto *btnStop = new QPushButton("停止");
    auto *btnRestart = new QPushButton("重启");
    auto *btnAdd = new QPushButton("添加桩");
    btnLayout->addWidget(btnStart);
    btnLayout->addWidget(btnStop);
    btnLayout->addWidget(btnRestart);
    btnLayout->addWidget(btnAdd);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(btnStart, &QPushButton::clicked, [this](){ onControl("start"); });
    connect(btnStop, &QPushButton::clicked, [this](){ onControl("stop"); });
    connect(btnRestart, &QPushButton::clicked, [this](){ onControl("restart"); });
    connect(btnAdd, &QPushButton::clicked, this, &ChargerManagementWidget::onAddCharger);
    connect(m_net, &NetworkManager::responseReceived, this, &ChargerManagementWidget::onResponse);

    loadChargers();
}

void ChargerManagementWidget::loadChargers()
{
    QString keyword = editKeyword->text().trimmed();
    QString status = comboStatus->currentText();
    if (status == "全部") status = "";
    // 暂未实现关键词搜索，可后续扩展
    m_net->getChargerList(1, 50, status);
}

void ChargerManagementWidget::onSearch() { loadChargers(); }

void ChargerManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::CHARGER_LIST && resp["status"].toString() == "ok") {
        auto list = resp["data"].toObject()["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["serial_number"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["status"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["power_kw"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["temperature"].toString()));
            table->setItem(i, 5, new QTableWidgetItem(obj["location"].toString()));
            table->setItem(i, 6, new QTableWidgetItem("操作"));
        }
    }
}

void ChargerManagementWidget::onControl(const QString &command)
{
    int row = table->currentRow();
    if (row < 0) return;
    int chargerId = table->item(row, 0)->text().toInt();
    m_net->controlCharger(chargerId, command);
    QMessageBox::information(this, "控制", "命令已发送: " + command);
    loadChargers();
}

void ChargerManagementWidget::onAddCharger()
{
    bool ok;
    QString serial = QInputDialog::getText(this, "添加充电桩", "序列号:", QLineEdit::Normal, "", &ok);
    if (!ok || serial.isEmpty()) return;
    double power = QInputDialog::getDouble(this, "添加充电桩", "功率(kW):", 60, 0, 500, 1, &ok);
    if (!ok) return;
    QString location = QInputDialog::getText(this, "添加充电桩", "位置:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QJsonObject data;
    data["serial_number"] = serial;
    data["power_kw"] = power;
    data["location"] = location;
    m_net->controlCharger(0, "add"); // 实际调用 add 需改服务端，这里简化
    loadChargers();
}
