/**
 * @file adminmain.cpp
 * @brief 管理端 - 主程序入口
 * 
 * 功能：系统配置、数据统计、医生管理、报表导出
 */

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QDockWidget>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QChartView>
#include <QPieSeries>
#include <QLineSeries>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QIcon>
#include <QFont>
#include <QPalette>
#include <QScreen>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBarSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QCategoryAxis>

#include "smartsched/client/networkclient.h"
#include "smartsched/common/version.h"

// =============================================================================
// 主窗口类
// =============================================================================
class AdminDashboard : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminDashboard(QWidget* parent = nullptr);
    ~AdminDashboard() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 连接相关
    void onConnect();
    void onDisconnect();
    void updateConnectionStatus(bool connected);
    
    // 数据管理
    void onRefreshAll();
    void onExportReport();
    void onBackupDatabase();
    
    // 医生管理
    void onAddDoctor();
    void onEditDoctor();
    void onDeleteDoctor();
    
    // 科室管理
    void onAddDepartment();
    void onEditDepartment();
    
    // 统计相关
    void onRefreshStatistics();
    void onGenerateChart();
    
    // 帮助
    void onAbout();
    void onHelp();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockPanels();
    
    // 加载数据
    void loadDepartments();
    void loadDoctors();
    void loadStatistics();
    
    // 网络客户端
    smartsched::client::NetworkClient* networkClient_;
    
    // UI组件
    QLabel* connectionLabel_;
    QLabel* timeLabel_;
    QTimer* timeTimer_;
    
    // 左侧导航
    QTreeWidget* navigationTree_;
    
    // 主内容区
    QWidget* contentWidget_;
    QStackedWidget* contentStack_;
    
    // 统计面板
    QLabel* totalPatientsLabel_;
    QLabel* todayRegistrationsLabel_;
    QLabel* todayConsultationsLabel_;
    QLabel* avgWaitTimeLabel_;
    QLabel* queueSizeLabel_;
    
    // 数据表格
    QTableWidget* doctorsTable_;
    QTableWidget* departmentsTable_;
    QTableWidget* historyTable_;
};

AdminDashboard::AdminDashboard(QWidget* parent)
    : QMainWindow(parent)
    , networkClient_(nullptr)
{
    setWindowTitle("智序医院 - 管理系统");
    setMinimumSize(1280, 800);
    resize(1600, 1000);
    
    // 网络客户端
    networkClient_ = new smartsched::client::NetworkClient(this);
    
    setupUi();
    
    // 时间更新
    timeTimer_ = new QTimer(this);
    connect(timeTimer_, &QTimer::timeout, this, [this]() {
        timeLabel_->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    timeTimer_->start(1000);
    
    // 加载初始数据
    loadDepartments();
    loadDoctors();
    loadStatistics();
    
    // 自动连接
    onConnect();
}

AdminDashboard::~AdminDashboard() {}

// =============================================================================
// UI初始化
// =============================================================================
void AdminDashboard::setupUi() {
    QFont defaultFont("Microsoft YaHei, SimHei, sans-serif");
    QApplication::setFont(defaultFont);
    
    // 中心部件布局
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // ========== 左侧导航 ==========
    QWidget* leftPanel = new QWidget();
    leftPanel->setMaximumWidth(220);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(5, 10, 5, 10);
    
    // 导航树
    navigationTree_ = new QTreeWidget();
    navigationTree_->setHeaderHidden(true);
    navigationTree_->setIndentation(15);
    
    QTreeWidgetItem* dashboardItem = new QTreeWidgetItem({"📊 控制面板"});
    QTreeWidgetItem* patientItem = new QTreeWidgetItem({"👥 患者管理"});
    QTreeWidgetItem* doctorItem = new QTreeWidgetItem({"👨‍⚕️ 医生管理"});
    QTreeWidgetItem* deptItem = new QTreeWidgetItem({"🏥 科室管理"});
    QTreeWidgetItem* queueItem = new QTreeWidgetItem({"📋 队列管理"});
    QTreeWidgetItem* statItem = new QTreeWidgetItem({"📈 统计报表"});
    QTreeWidgetItem* configItem = new QTreeWidgetItem({"⚙️ 系统设置"});
    
    navigationTree_->addTopLevelItems({
        dashboardItem, patientItem, doctorItem, deptItem,
        queueItem, statItem, configItem
    });
    
    leftLayout->addWidget(navigationTree_);
    
    mainSplitter->addWidget(leftPanel);
    
    // ========== 主内容区 ==========
    QWidget* centerWidget = new QWidget();
    QVBoxLayout* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(10, 10, 10, 10);
    
    // Tab页面
    QTabWidget* mainTabs = new QTabWidget();
    
    // ---- 控制面板 ----
    QWidget* dashboardPage = new QWidget();
    QGridLayout* dashboardLayout = new QGridLayout(dashboardPage);
    
    // 统计卡片行1
    QGroupBox* statsBox1 = new QGroupBox("今日概览");
    QHBoxLayout* statsRow1 = new QHBoxLayout(statsBox1);
    
    // 卡片1: 挂号数
    QVBoxLayout* card1 = createStatCard("今日挂号", "156", "#E3F2FD", "#1976D2");
    statsRow1->addLayout(card1);
    
    // 卡片2: 就诊数
    QVBoxLayout* card2 = createStatCard("今日就诊", "142", "#E8F5E9", "#4CAF50");
    statsRow1->addLayout(card2);
    
    // 卡片3: 当前排队
    QVBoxLayout* card3 = createStatCard("当前排队", "89", "#FFF3E0", "#FF9800");
    statsRow1->addLayout(card3);
    
    // 卡片4: 平均等待
    QVBoxLayout* card4 = createStatCard("平均等待", "12分钟", "#FCE4EC", "#E91E63");
    statsRow1->addLayout(card4);
    
    dashboardLayout->addWidget(statsBox1, 0, 0, 1, 2);
    
    // 统计卡片行2
    QGroupBox* statsBox2 = new QGroupBox("实时状态");
    QHBoxLayout* statsRow2 = new QHBoxLayout(statsBox2);
    
    statsRow2->addWidget(new QLabel("🟢 在线终端: 12"));
    statsRow2->addWidget(new QLabel("🟡 候诊人数: 89"));
    statsRow2->addWidget(new QLabel("🔬 B超可用: 2/3"));
    statsRow2->addStretch();
    
    dashboardLayout->addWidget(statsBox2, 1, 0, 1, 2);
    
    // 科室排队情况
    QGroupBox* deptQueueBox = new QGroupBox("各科室排队情况");
    QVBoxLayout* deptQueueLayout = new QVBoxLayout(deptQueueBox);
    
    QTableWidget* deptQueueTable = new QTableWidget();
    deptQueueTable->setColumnCount(4);
    deptQueueTable->setHorizontalHeaderLabels({"科室", "当前排队", "今日挂号", "平均等待"});
    deptQueueTable->setAlternatingRowColors(true);
    deptQueueTable->verticalHeader()->setVisible(false);
    
    QStringList depts = {"内科", "外科", "儿科", "妇科", "骨科", "神经内科", "心血管内科"};
    for (int i = 0; i < depts.size(); ++i) {
        deptQueueTable->insertRow(i);
        deptQueueTable->setItem(i, 0, new QTableWidgetItem(depts[i]));
        deptQueueTable->setItem(i, 1, new QTableWidgetItem(QString::number(5 + i * 3)));
        deptQueueTable->setItem(i, 2, new QTableWidgetItem(QString::number(20 + i * 5)));
        deptQueueTable->setItem(i, 3, new QTableWidgetItem(QString::number(8 + i) + "分钟"));
    }
    
    deptQueueTable->setColumnWidth(0, 150);
    deptQueueTable->setColumnWidth(1, 120);
    deptQueueTable->setColumnWidth(2, 120);
    deptQueueTable->setColumnWidth(3, 120);
    
    deptQueueLayout->addWidget(deptQueueTable);
    dashboardLayout->addWidget(deptQueueBox, 2, 0);
    
    // 今日接诊排名
    QGroupBox* rankingBox = new QGroupBox("今日接诊排名");
    QVBoxLayout* rankingLayout = new QVBoxLayout(rankingBox);
    
    QTableWidget* rankingTable = new QTableWidget();
    rankingTable->setColumnCount(3);
    rankingTable->setHorizontalHeaderLabels({"排名", "医生", "接诊数"});
    rankingTable->setAlternatingRowColors(true);
    rankingTable->verticalHeader()->setVisible(false);
    
    QStringList doctors = {"张明华", "李秀英", "王建国", "赵伟东", "孙丽娟"};
    for (int i = 0; i < doctors.size(); ++i) {
        rankingTable->insertRow(i);
        rankingTable->setItem(i, 0, new QTableWidgetItem(QString("#%1").arg(i + 1)));
        rankingTable->setItem(i, 1, new QTableWidgetItem(doctors[i]));
        rankingTable->setItem(i, 2, new QTableWidgetItem(QString::number(25 - i * 2)));
    }
    
    rankingLayout->addWidget(rankingTable);
    dashboardLayout->addWidget(rankingBox, 2, 1);
    
    mainTabs->addTab(dashboardPage, "控制面板");
    
    // ---- 医生管理 ----
    QWidget* doctorPage = new QWidget();
    QVBoxLayout* doctorLayout = new QVBoxLayout(doctorPage);
    
    QHBoxLayout* doctorButtons = new QHBoxLayout();
    doctorButtons->addWidget(new QPushButton("添加医生"));
    doctorButtons->addWidget(new QPushButton("编辑医生"));
    doctorButtons->addWidget(new QPushButton("删除医生"));
    doctorButtons->addStretch();
    doctorButtons->addWidget(new QPushButton("刷新"));
    
    doctorLayout->addLayout(doctorButtons);
    
    doctorsTable_ = new QTableWidget();
    doctorsTable_->setColumnCount(6);
    doctorsTable_->setHorizontalHeaderLabels({"工号", "姓名", "科室", "职称", "专长", "状态"});
    doctorsTable_->setAlternatingRowColors(true);
    doctorsTable_->verticalHeader()->setVisible(false);
    
    // 模拟数据
    QStringList doctorNames = {"张明华", "李秀英", "王建国", "赵伟东", "孙丽娟", "周海涛"};
    QStringList titles = {"主任医师", "副主任医师", "主治医师", "主任医师", "副主任医师", "主治医师"};
    QStringList specialties = {"心血管疾病", "消化系统", "呼吸系统", "微创手术", "乳腺疾病", "普外"};
    
    for (int i = 0; i < doctorNames.size(); ++i) {
        doctorsTable_->insertRow(i);
        doctorsTable_->setItem(i, 0, new QTableWidgetItem(QString("D%1").arg(i + 1, 4, 10, QChar('0'))));
        doctorsTable_->setItem(i, 1, new QTableWidgetItem(doctorNames[i]));
        doctorsTable_->setItem(i, 2, new QTableWidgetItem("内科"));
        doctorsTable_->setItem(i, 3, new QTableWidgetItem(titles[i]));
        doctorsTable_->setItem(i, 4, new QTableWidgetItem(specialties[i]));
        doctorsTable_->setItem(i, 5, new QTableWidgetItem("🟢 在职"));
    }
    
    doctorsTable_->setColumnWidth(0, 100);
    doctorsTable_->setColumnWidth(1, 100);
    doctorsTable_->setColumnWidth(2, 100);
    doctorsTable_->setColumnWidth(3, 120);
    doctorsTable_->setColumnWidth(4, 200);
    doctorsTable_->setColumnWidth(5, 80);
    
    doctorLayout->addWidget(doctorsTable_);
    
    mainTabs->addTab(doctorPage, "医生管理");
    
    // ---- 统计报表 ----
    QWidget* statPage = new QWidget();
    QVBoxLayout* statLayout = new QVBoxLayout(statPage);
    
    QHBoxLayout* statFilters = new QHBoxLayout();
    statFilters->addWidget(new QLabel("日期范围:"));
    statFilters->addWidget(new QDateEdit());
    statFilters->addWidget(new QLabel("至"));
    statFilters->addWidget(new QDateEdit());
    statFilters->addWidget(new QComboBox());
    statFilters->addStretch();
    statFilters->addWidget(new QPushButton("查询"));
    statFilters->addWidget(new QPushButton("导出Excel"));
    
    statLayout->addLayout(statFilters);
    
    // 统计表格
    historyTable_ = new QTableWidget();
    historyTable_->setColumnCount(7);
    historyTable_->setHorizontalHeaderLabels({"日期", "科室", "挂号数", "就诊数", "平均等待", "B超检查", "完成率"});
    historyTable_->setAlternatingRowColors(true);
    historyTable_->verticalHeader()->setVisible(false);
    
    statLayout->addWidget(historyTable_);
    
    mainTabs->addTab(statPage, "统计报表");
    
    centerLayout->addWidget(mainTabs);
    mainSplitter->addWidget(centerWidget);
    
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    
    setCentralWidget(mainSplitter);
    
    // 状态栏
    statusBar()->showMessage("就绪");
    
    QLabel* spacer = new QLabel();
    statusBar()->addPermanentWidget(spacer, 1);
    
    connectionLabel_ = new QLabel("⚫ 未连接");
    statusBar()->addPermanentWidget(connectionLabel_);
    
    statusBar()->addPermanentWidget(new QLabel("  |  "));
    
    timeLabel_ = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    statusBar()->addPermanentWidget(timeLabel_);
}

// 创建统计卡片
QVBoxLayout* createStatCard(const QString& title, const QString& value, const QString& bgColor, const QString& textColor) {
    QVBoxLayout* layout = new QVBoxLayout();
    
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("font-size: 14px; color: #616161;"));
    layout->addWidget(titleLabel);
    
    QLabel* valueLabel = new QLabel(value);
    valueLabel->setAlignment(Qt::AlignCenter);
    valueLabel->setStyleSheet(QString("font-size: 32px; font-weight: bold; color: %1;").arg(textColor));
    layout->addWidget(valueLabel);
    
    return layout;
}

void AdminDashboard::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    
    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("刷新数据", this, &AdminDashboard::onRefreshAll, QKeySequence::Refresh);
    fileMenu->addSeparator();
    fileMenu->addAction("导出报表", this, &AdminDashboard::onExportReport);
    fileMenu->addAction("备份数据库", this, &AdminDashboard::onBackupDatabase);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close, QKeySequence::Quit);
    
    // 医生管理菜单
    QMenu* doctorMenu = menuBar->addMenu("医生(&D)");
    doctorMenu->addAction("添加医生", this, &AdminDashboard::onAddDoctor);
    doctorMenu->addAction("编辑医生", this, &AdminDashboard::onEditDoctor);
    doctorMenu->addAction("删除医生", this, &AdminDashboard::onDeleteDoctor);
    
    // 科室管理菜单
    QMenu* deptMenu = menuBar->addMenu("科室(&D)");
    deptMenu->addAction("添加科室", this, &AdminDashboard::onAddDepartment);
    deptMenu->addAction("编辑科室", this, &AdminDashboard::onEditDepartment);
    
    // 统计菜单
    QMenu* statMenu = menuBar->addMenu("统计(&S)");
    statMenu->addAction("刷新统计", this, &AdminDashboard::onRefreshStatistics);
    statMenu->addAction("生成图表", this, &AdminDashboard::onGenerateChart);
    
    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu("帮助(&H)");
    helpMenu->addAction("使用手册", this, &AdminDashboard::onHelp);
    helpMenu->addSeparator();
    helpMenu->addAction("关于", this, &AdminDashboard::onAbout);
}

// =============================================================================
// 数据加载
// =============================================================================
void AdminDashboard::loadDepartments() {
    // 模拟加载科室数据
}

void AdminDashboard::loadDoctors() {
    // 模拟加载医生数据
}

void AdminDashboard::loadStatistics() {
    // 模拟加载统计数据
}

// =============================================================================
// 槽函数
// =============================================================================
void AdminDashboard::onConnect() {
    networkClient_->connectToServer();
}

void AdminDashboard::onDisconnect() {
    networkClient_->disconnectFromServer();
}

void AdminDashboard::updateConnectionStatus(bool connected) {
    connectionLabel_->setText(connected ? "🟢 已连接" : "⚫ 未连接");
}

void AdminDashboard::onRefreshAll() {
    loadDepartments();
    loadDoctors();
    loadStatistics();
    statusBar()->showMessage("数据已刷新", 3000);
}

void AdminDashboard::onExportReport() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出报表",
        QString("统计报表_%1.xlsx").arg(QDate::currentDate().toString("yyyyMMdd")),
        "Excel文件 (*.xlsx)");
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, "导出成功", 
            QString("报表已导出至: %1").arg(fileName));
    }
}

void AdminDashboard::onBackupDatabase() {
    QMessageBox::information(this, "备份", "数据库备份功能（待实现）");
}

void AdminDashboard::onAddDoctor() {
    QMessageBox::information(this, "添加医生", "添加医生对话框（待实现）");
}

void AdminDashboard::onEditDoctor() {
    QMessageBox::information(this, "编辑医生", "编辑医生对话框（待实现）");
}

void AdminDashboard::onDeleteDoctor() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "删除确认",
        "确定要删除选中的医生吗？",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 执行删除
    }
}

void AdminDashboard::onAddDepartment() {
    QMessageBox::information(this, "添加科室", "添加科室对话框（待实现）");
}

void AdminDashboard::onEditDepartment() {
    QMessageBox::information(this, "编辑科室", "编辑科室对话框（待实现）");
}

void AdminDashboard::onRefreshStatistics() {
    loadStatistics();
    statusBar()->showMessage("统计数据已刷新", 3000);
}

void AdminDashboard::onGenerateChart() {
    QMessageBox::information(this, "生成图表", "图表生成功能（待实现）");
}

void AdminDashboard::onAbout() {
    QMessageBox::about(this, "关于",
        QString("<h3>智序医院 - 管理系统</h3>"
                "<p>版本: %1</p>"
                "<p>© 2024-2026 SmartSched Healthcare Technology</p>").arg(SMARTSCHED_VERSION_STRING));
}

void AdminDashboard::onHelp() {
    QMessageBox::information(this, "帮助", "使用手册（待实现）");
}

void AdminDashboard::closeEvent(QCloseEvent* event) {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认退出",
        "确定要退出管理系统吗?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}

// =============================================================================
// 主函数
// =============================================================================
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("SmartSched-管理系统");
    app.setApplicationVersion(SMARTSCHED_VERSION_STRING);
    app.setOrganizationName("SmartSched Healthcare");
    
    AdminDashboard window;
    window.show();
    
    return app.exec();
}

#include "adminmain.moc"
