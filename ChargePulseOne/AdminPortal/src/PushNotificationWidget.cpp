#include "PushNotificationWidget.h"
#include "NetworkManager.h"
#include "common.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>

PushNotificationWidget::PushNotificationWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // 标题
    auto *headerLayout = new QHBoxLayout;
    QLabel *title = new QLabel("📢 消息推送管理");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    // 操作按钮
    QPushButton *btnSend = new QPushButton("📤 发送推送");
    btnSend->setStyleSheet("QPushButton { padding: 8px 20px; background: #4A90D9; color: white; border: none; border-radius: 4px; }");
    connect(btnSend, &QPushButton::clicked, this, &PushNotificationWidget::onSendNotification);
    headerLayout->addWidget(btnSend);

    QPushButton *btnDelete = new QPushButton("🗑️ 删除");
    btnDelete->setStyleSheet("QPushButton { padding: 8px 20px; background: #F44336; color: white; border: none; border-radius: 4px; }");
    connect(btnDelete, &QPushButton::clicked, this, &PushNotificationWidget::onDeleteNotification);
    headerLayout->addWidget(btnDelete);

    QPushButton *btnRefresh = new QPushButton("🔄 刷新");
    btnRefresh->setStyleSheet("QPushButton { padding: 8px 20px; background: #4CAF50; color: white; border: none; border-radius: 4px; }");
    connect(btnRefresh, &QPushButton::clicked, this, &PushNotificationWidget::onRefresh);
    headerLayout->addWidget(btnRefresh);
    mainLayout->addLayout(headerLayout);

    // 推送列表
    table = new QTableWidget;
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID", "标题", "内容", "类型", "发送时间", "状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(table);

    connect(m_net, &NetworkManager::responseReceived, this, &PushNotificationWidget::onResponse);

    loadNotifications();
}

void PushNotificationWidget::loadNotifications()
{
    // 模拟推送数据
    QStringList titles = {"系统升级通知", "充电桩维护公告", "会员日活动", "新版APP发布", "优惠活动提醒"};
    QStringList contents = {
        "系统将于今晚22:00-24:00进行升级维护",
        "A区充电桩将于本周六进行例行维护",
        "本月会员日充电可享8折优惠",
        "ChargePulse APP v2.0正式发布",
        "新用户首充满100元送50元优惠券"
    };
    QStringList types = {"系统", "公告", "活动", "更新", "优惠"};
    QStringList statuses = {"已发送", "已发送", "已发送", "已发送", "待发送"};

    table->setRowCount(titles.size());
    for (int i = 0; i < titles.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(1000 + i)));
        table->setItem(i, 1, new QTableWidgetItem(titles[i]));
        table->setItem(i, 2, new QTableWidgetItem(contents[i]));
        table->setItem(i, 3, new QTableWidgetItem(types[i]));
        table->setItem(i, 4, new QTableWidgetItem("2024-01-" + QString::number(10 + i).rightJustified(2, '0') + " 10:00"));
        table->setItem(i, 5, new QTableWidgetItem(statuses[i]));
    }
}

void PushNotificationWidget::onSendNotification()
{
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("发送推送消息");
    dialog->setMinimumSize(500, 300);

    auto *layout = new QVBoxLayout(dialog);

    layout->addWidget(new QLabel("标题:"));
    auto *titleEdit = new QLineEdit;
    titleEdit->setPlaceholderText("输入推送标题");
    layout->addWidget(titleEdit);

    layout->addWidget(new QLabel("内容:"));
    auto *contentEdit = new QTextEdit;
    contentEdit->setPlaceholderText("输入推送内容");
    layout->addWidget(contentEdit);

    layout->addWidget(new QLabel("类型:"));
    auto *typeCombo = new QComboBox;
    typeCombo->addItems({"全部用户", "指定用户", "黄金会员", "铂金会员", "新用户"});
    layout->addWidget(typeCombo);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    if (dialog->exec() == QDialog::Accepted) {
        QString title = titleEdit->text().trimmed();
        QString content = contentEdit->toPlainText().trimmed();

        if (title.isEmpty() || content.isEmpty()) {
            QMessageBox::warning(this, "提示", "标题和内容不能为空");
            return;
        }

        // 发送推送请求
        QJsonObject req;
        req["cmd"] = CMD::PUSH_NOTIFICATION;
        req["title"] = title;
        req["content"] = content;
        req["type"] = typeCombo->currentText();
        m_net->sendRequest(req);

        QMessageBox::information(this, "发送成功", "推送消息已提交发送");
        loadNotifications();
    }
}

void PushNotificationWidget::onDeleteNotification()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的推送记录");
        return;
    }

    int id = table->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, "确认删除", "确定要删除这条推送记录吗？") == QMessageBox::Yes) {
        // 发送删除请求
        QJsonObject req;
        req["cmd"] = CMD::DELETE_NOTIFICATION;
        req["id"] = id;
        m_net->sendRequest(req);

        table->removeRow(row);
        QMessageBox::information(this, "删除成功", "推送记录已删除");
    }
}

void PushNotificationWidget::onRefresh()
{
    loadNotifications();
}

void PushNotificationWidget::onResponse(const QJsonObject &resp)
{
    if (resp["status"].toString() == "ok") {
        // 处理响应
    }
}
