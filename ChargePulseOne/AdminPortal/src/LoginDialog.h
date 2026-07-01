#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
class NetworkManager;

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onLogin();
    void onResponse(const QJsonObject &resp);
    void onError(const QString &err);

private:
    NetworkManager *m_net;
    QLineEdit *editUser;
    QLineEdit *editPass;
    QPushButton *btnLogin;
    QLabel *statusLabel;
};
