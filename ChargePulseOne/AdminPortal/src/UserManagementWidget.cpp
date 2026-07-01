#include <QJsonArray>
#include <QJsonArray>
#include "common.h"
#include "UserManagementWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonArray>

UserManagementWidget::UserManagementWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);

    // 搜索栏
    auto *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("关键词:"));
    editKeyword = new QLineEdit;
    editKeyword->setPlaceholderText("用户名/手机号");
    searchLayout->addWidget(editKeyword);
    searchLayout->addWidget(new QLabel("角色:"));
    comboRole = new QComboBox;
    comboRole->addItems({"全部", "driver", "operator", "admin"});
    searchLayout->addWidget(comboRole);
    searchLayout->addWidget(new QLabel("状态:"));
    comboStatus = new QComboBox;
    comboStatus->addItems({"全部", "active", "disabled", "blacklisted"});
    searchLayout->addWidget(comboStatus);
    auto *btnSearch = new QPushButton("搜索");
    connect(btnSearch, &QPushButton::clicked, this, &UserManagementWidget::onSearch);
    searchLayout->addWidget(btnSearch);
    mainLayout->addLayout(searchLayout);

    // 表格
    table = new QTableWidget;
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "用户名", "角色", "手机", "车牌", "状态", "操作"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);

    // 按钮
    auto *btnLayout = new QHBoxLayout;
    auto *btnEdit = new QPushButton("编辑");
    auto *btnToggle = new QPushButton("禁用/启用");
    auto *btnReset = new QPushButton("重置密码");
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnToggle);
    btnLayout->addWidget(btnReset);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    connect(btnEdit, &QPushButton::clicked, this, &UserManagementWidget::onEditUser);
    connect(btnToggle, &QPushButton::clicked, this, &UserManagementWidget::onToggleStatus);
    connect(btnReset, &QPushButton::clicked, this, &UserManagementWidget::onResetPassword);
    connect(m_net, &NetworkManager::responseReceived, this, &UserManagementWidget::onResponse);

    loadUsers();
}

void UserManagementWidget::loadUsers()
{
    QString keyword = editKeyword->text().trimmed();
    QString role = comboRole->currentText();
    if (role == "全部") role = "";
    QString status = comboStatus->currentText();
    if (status == "全部") status = "";
    m_net->getUserList(currentPage, 20, keyword, role, status);
}

void UserManagementWidget::onSearch() { currentPage = 1; loadUsers(); }

void UserManagementWidget::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::USER_LIST && resp["status"].toString() == "ok") {
        auto data = resp["data"].toObject();
        auto list = data["list"].toArray();
        table->setRowCount(list.size());
        for (int i = 0; i < list.size(); ++i) {
            auto obj = list[i].toObject();
            table->setItem(i, 0, new QTableWidgetItem(obj["id"].toString()));
            table->setItem(i, 1, new QTableWidgetItem(obj["username"].toString()));
            table->setItem(i, 2, new QTableWidgetItem(obj["role"].toString()));
            table->setItem(i, 3, new QTableWidgetItem(obj["phone"].toString()));
            table->setItem(i, 4, new QTableWidgetItem(obj["plate_number"].toString()));
            table->setItem(i, 5, new QTableWidgetItem(obj["status"].toString()));
            table->setItem(i, 6, new QTableWidgetItem("操作"));
        }
    }
}

void UserManagementWidget::onEditUser()
{
    int row = table->currentRow();
    if (row < 0) return;
    int userId = table->item(row, 0)->text().toInt();
    // 简单弹出对话框，可扩展
    bool ok;
    QString status = QInputDialog::getText(this, "编辑用户", "新状态 (active/disabled/blacklisted):",
                                           QLineEdit::Normal, table->item(row, 5)->text(), &ok);
    if (ok && !status.isEmpty()) {
        QJsonObject data;
        data["status"] = status;
        m_net->editUser(userId, data);
    }
}

void UserManagementWidget::onToggleStatus()
{
    int row = table->currentRow();
    if (row < 0) return;
    int userId = table->item(row, 0)->text().toInt();
    QString curStatus = table->item(row, 5)->text();
    QString newStatus = (curStatus == "active") ? "disabled" : "active";
    QJsonObject data;
    data["status"] = newStatus;
    m_net->editUser(userId, data);
    loadUsers(); // 刷新
}

void UserManagementWidget::onResetPassword()
{
    int row = table->currentRow();
    if (row < 0) return;
    int userId = table->item(row, 0)->text().toInt();
    bool ok;
    QString newPwd = QInputDialog::getText(this, "重置密码", "新密码:",
                                           QLineEdit::Password, "", &ok);
    if (ok && !newPwd.isEmpty()) {
        QJsonObject data;
        data["new_password"] = newPwd;
        m_net->editUser(userId, data);
        QMessageBox::information(this, "成功", "密码已重置");
    }
}
