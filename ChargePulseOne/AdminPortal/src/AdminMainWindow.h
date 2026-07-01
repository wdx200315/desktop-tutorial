#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QJsonObject>
#include <QTimer>
class NetworkManager;

class AdminMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AdminMainWindow(NetworkManager *net, QWidget *parent = nullptr);
    ~AdminMainWindow();

private slots:
    void onNavItemClicked(int index);
    void onResponse(const QJsonObject &resp);
    void onLogin();
    void onLogout();
    void onRefresh();

private:
    NetworkManager *m_net;
    QStackedWidget *m_stack;
    QListWidget *m_navList;
    QWidget *m_loginWidget;
    QWidget *m_mainWidget;
    QLabel *m_userLabel;

    // 登录相关
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_loginStatus;

    bool m_isLoggedIn = false;

    void createLoginPage();
    void createMainPage();
    void createNavigation();
    void showMainInterface();
    void showLoginInterface();
};
