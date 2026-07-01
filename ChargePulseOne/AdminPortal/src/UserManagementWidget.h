#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
class NetworkManager;

class UserManagementWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UserManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();
    void onEditUser();
    void onToggleStatus();
    void onResetPassword();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QLineEdit *editKeyword;
    QComboBox *comboRole;
    QComboBox *comboStatus;
    int currentPage = 1;
    void loadUsers();
    void showEditDialog(int row);
};
