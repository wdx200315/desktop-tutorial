#include "common.h"
#include "LoginDialog.h"
#include "NetworkManager.h"
#include <QMessageBox>

LoginDialog::LoginDialog(NetworkManager *net, QWidget *parent)
    : QDialog(parent), m_net(net)
{
    setWindowTitle("管理端登录");
    setFixedSize(300, 180);

    editUser = new QLineEdit;
    editUser->setPlaceholderText("用户名");
    editPass = new QLineEdit;
    editPass->setPlaceholderText("密码");
    editPass->setEchoMode(QLineEdit::Password);
    btnLogin = new QPushButton("登录");
    statusLabel = new QLabel;
    statusLabel->setStyleSheet("color: red;");

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(editUser);
    layout->addWidget(editPass);
    layout->addWidget(btnLogin);
    layout->addWidget(statusLabel);

    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_net, &NetworkManager::responseReceived, this, &LoginDialog::onResponse);
    connect(m_net, &NetworkManager::errorOccurred, this, &LoginDialog::onError);

    // 尝试连接服务端（默认127.0.0.1:9010）
    m_net->connectToServer("127.0.0.1", 9010);
}

void LoginDialog::onLogin()
{
    QString user = editUser->text().trimmed();
    QString pass = editPass->text().trimmed();
    if (user.isEmpty() || pass.isEmpty()) {
        statusLabel->setText("请输入用户名和密码");
        return;
    }
    btnLogin->setEnabled(false);
    statusLabel->setText("登录中...");
    m_net->login(user, pass);
}

void LoginDialog::onResponse(const QJsonObject &resp)
{
    if (resp["cmd"].toString() == CMD::LOGIN) {
        if (resp["status"].toString() == "ok") {
            QString role = resp["data"].toObject()["role"].toString();
            if (role == "admin" || role == "operator") {
                accept();
            } else {
                statusLabel->setText("权限不足，仅管理员/运营商可登录");
                btnLogin->setEnabled(true);
            }
        } else {
            statusLabel->setText(resp["message"].toString());
            btnLogin->setEnabled(true);
        }
    }
}

void LoginDialog::onError(const QString &err)
{
    statusLabel->setText("网络错误: " + err);
    btnLogin->setEnabled(true);
}
