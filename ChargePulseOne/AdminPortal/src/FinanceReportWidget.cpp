#include "common.h"
#include "FinanceReportWidget.h"
#include "NetworkManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

FinanceReportWidget::FinanceReportWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent), m_net(net)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // 标题
    QLabel *title = new QLabel("💰 财务统计报表");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    mainLayout->addWidget(title);

    // 统计卡片
    auto *statsGroup = new QGroupBox("核心财务指标");
    auto *statsGrid = new QGridLayout(statsGroup);

    lblTotalRevenue = new QLabel("¥0.00");
    lblTotalRevenue->setStyleSheet("font-size: 28px; font-weight: bold; color: #4CAF50;");
    lblTotalOrders = new QLabel("0");
    lblTotalOrders->setStyleSheet("font-size: 28px; font-weight: bold; color: #4A90D9;");
    lblAvgOrderValue = new QLabel("¥0.00");
    lblAvgOrderValue->setStyleSheet("font-size: 28px; font-weight: bold; color: #FF9800;");
    lblTotalEnergy = new QLabel("0 kWh");
    lblTotalEnergy->setStyleSheet("font-size: 28px; font-weight: bold; color: #9C27B0;");

    statsGrid->addWidget(new QLabel("总收入"), 0, 0, Qt::AlignCenter);
    statsGrid->addWidget(lblTotalRevenue, 1, 0, Qt::AlignCenter);
    statsGrid->addWidget(new QLabel("总订单数"), 0, 1, Qt::AlignCenter);
    statsGrid->addWidget(lblTotalOrders, 1, 1, Qt::AlignCenter);
    statsGrid->addWidget(new QLabel("平均订单金额"), 0, 2, Qt::AlignCenter);
    statsGrid->addWidget(lblAvgOrderValue, 1, 2, Qt::AlignCenter);
    statsGrid->addWidget(new QLabel("总充电量"), 0, 3, Qt::AlignCenter);
    statsGrid->addWidget(lblTotalEnergy, 1, 3, Qt::AlignCenter);
    mainLayout->addWidget(statsGroup);

    // 日期选择
    auto *dateLayout = new QHBoxLayout;
    dateLayout->addWidget(new QLabel("选择月份:"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM");
    dateLayout->addWidget(dateEdit);

    QPushButton *btnLoad = new QPushButton("加载报表");
    btnLoad->setStyleSheet("QPushButton { padding: 6px 16px; background: #4A90D9; color: white; border: none; border-radius: 4px; }");
    connect(btnLoad, &QPushButton::clicked, this, &FinanceReportWidget::loadData);
    dateLayout->addWidget(btnLoad);

    QPushButton *btnExport = new QPushButton("导出CSV");
    btnExport->setStyleSheet("QPushButton { padding: 6px 16px; background: #4CAF50; color: white; border: none; border-radius: 4px; }");
    connect(btnExport, &QPushButton::clicked, this, &FinanceReportWidget::onExport);
    dateLayout->addWidget(btnExport);

    QPushButton *btnRefresh = new QPushButton("刷新");
    connect(btnRefresh, &QPushButton::clicked, this, &FinanceReportWidget::onRefresh);
    dateLayout->addWidget(btnRefresh);
    dateLayout->addStretch();
    mainLayout->addLayout(dateLayout);

    // 详细报表表格
    table = new QTableWidget;
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"日期", "订单数", "充电量(kWh)", "电费收入", "服务费收入", "总金额", "成本", "利润"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);

    connect(m_net, &NetworkManager::responseReceived, this, &FinanceReportWidget::onResponse);

    loadData();
}

void FinanceReportWidget::loadData()
{
    // 模拟财务数据
    double totalRevenue = 45680.50;
    int totalOrders = 1280;
    double avgOrder = totalOrders > 0 ? totalRevenue / totalOrders : 0;
    double totalEnergy = 15680.5;

    lblTotalRevenue->setText(QString("¥%1").arg(totalRevenue, 0, 'f', 2));
    lblTotalOrders->setText(QString::number(totalOrders));
    lblAvgOrderValue->setText(QString("¥%1").arg(avgOrder, 0, 'f', 2));
    lblTotalEnergy->setText(QString::number(totalEnergy, 'f', 1) + " kWh");

    // 生成每日数据
    QDate date = dateEdit->date();
    int days = date.daysInMonth();
    table->setRowCount(days);

    for (int i = 1; i <= days; ++i) {
        QDate day(date.year(), date.month(), i);
        int orders = 30 + (qrand() % 50);
        double energy = 400 + (qrand() % 200);
        double powerFee = energy * 0.8;
        double serviceFee = energy * 0.2;
        double total = powerFee + serviceFee;
        double cost = powerFee * 0.6;
        double profit = total - cost;

        table->setItem(i - 1, 0, new QTableWidgetItem(day.toString("yyyy-MM-dd")));
        table->setItem(i - 1, 1, new QTableWidgetItem(QString::number(orders)));
        table->setItem(i - 1, 2, new QTableWidgetItem(QString::number(energy, 'f', 1)));
        table->setItem(i - 1, 3, new QTableWidgetItem(QString("¥%1").arg(powerFee, 0, 'f', 2)));
        table->setItem(i - 1, 4, new QTableWidgetItem(QString("¥%1").arg(serviceFee, 0, 'f', 2)));
        table->setItem(i - 1, 5, new QTableWidgetItem(QString("¥%1").arg(total, 0, 'f', 2)));
        table->setItem(i - 1, 6, new QTableWidgetItem(QString("¥%1").arg(cost, 0, 'f', 2)));
        table->setItem(i - 1, 7, new QTableWidgetItem(QString("¥%1").arg(profit, 0, 'f', 2)));
    }
}

void FinanceReportWidget::onDaily()
{
    // 简化为调用 loadData
    loadData();
}

void FinanceReportWidget::onMonthly()
{
    loadData();
}

void FinanceReportWidget::onExport()
{
    QString path = QFileDialog::getSaveFileName(this, "导出财务报表",
                                               QString("财务报表_%1.csv").arg(dateEdit->date().toString("yyyy-MM")),
                                               "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法写入文件");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 写入统计摘要
    out << "财务统计报表," << dateEdit->date().toString("yyyy-MM") << "\n";
    out << "总收入," << lblTotalRevenue->text() << "\n";
    out << "总订单," << lblTotalOrders->text() << "\n";
    out << "平均订单金额," << lblAvgOrderValue->text() << "\n";
    out << "总充电量," << lblTotalEnergy->text() << "\n\n";

    // 写入表头
    for (int col = 0; col < table->columnCount(); ++col) {
        out << table->horizontalHeaderItem(col)->text() << ",";
    }
    out << "\n";

    // 写入数据
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            out << table->item(row, col)->text() << ",";
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "导出成功", QString("报表已导出至: %1").arg(path));
}

void FinanceReportWidget::onRefresh()
{
    loadData();
}

void FinanceReportWidget::onResponse(const QJsonObject &resp)
{
    // 处理服务端响应
}
