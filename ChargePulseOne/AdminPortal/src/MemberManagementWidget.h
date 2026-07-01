#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
class NetworkManager;

class MemberManagementWidget : public QWidget {
    Q_OBJECT
public:
    explicit MemberManagementWidget(NetworkManager *net, QWidget *parent = nullptr);

private slots:
    void onResponse(const QJsonObject &resp);
    void onSearch();
    void onAdjustLevel();
    void onRefresh();

private:
    NetworkManager *m_net;
    QTableWidget *table;
    QComboBox *comboLevel;
    QLabel *lblTotalMembers;
    QLabel *lblActiveMembers;
    QLabel *lblNewThisMonth;
    void loadMembers();
    void loadStats();
};
