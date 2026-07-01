/**
 * @file doctormain.cpp
 * @brief 医生工作站 - 主程序入口
 * 
 * 功能：医生接诊界面，查看患者信息，叫号，完成就诊
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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QSplitter>
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
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QIcon>
#include <QFont>
#include <QPalette>
#include <QScreen>
#include <QTranslator>
#include <QLocale>

#include "smartsched/client/networkclient.h"
#include "smartsched/client/datamodel.h"
#include "smartsched/common/version.h"

// =============================================================================
// 主窗口类
// =============================================================================
class DoctorWorkstation : public QMainWindow {
    Q_OBJECT

public:
    explicit DoctorWorkstation(QWidget* parent = nullptr);
    ~DoctorWorkstation() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 连接相关
    void onConnect();
    void onDisconnect();
    void onConnectionStateChanged(int state);
    
    // 叫号相关
    void onCallNext();
    void onCallSpecific();
    void onRecall();
    void onSkip();
    
    // 接诊相关
    void onStartConsultation();
    void onEndConsultation();
    void onPauseConsultation();
    
    // B超相关
    void onRequestUltrasound();
    
    // 数据刷新
    void onRefreshQueue();
    void onRefreshStatistics();
    
    // 设置相关
    void onSettings();
    void onAbout();
    
    // 网络消息
    void onMessageReceived(const QVariant& json);

private:
    void setupUi();
    void setupConnections();
    void setupTrayIcon();
    void updateConnectionStatus(bool connected);
    void showNotification(const QString& title, const QString& message);
    
    // 网络客户端
    smartsched::client::NetworkClient* networkClient_;
    
    // 医生信息
    int currentDoctorId_;
    QString currentDoctorName_;
    int currentPatientId_;
    
    // UI组件
    QLabel* statusLabel_;
    QLabel* connectionLabel_;
    QLabel* timeLabel_;
    QTimer* timeTimer_;
    
    QTableWidget* queueTable_;
    QTableWidget* historyTable_;
    
    QGroupBox* currentPatientBox_;
    QLabel* patientNameLabel_;
    QLabel* patientAgeLabel_;
    QLabel* patientGenderLabel_;
    QLabel* patientPhoneLabel_;
    QTextEdit* patientHistoryEdit_;
    
    QGroupBox* statisticsBox_;
    QLabel* todayCountLabel_;
    QLabel* avgWaitTimeLabel_;
    QLabel* currentQueueLabel_;
    
    QPushButton* callNextBtn_;
    QPushButton* callSpecificBtn_;
    QPushButton* recallBtn_;
    QPushButton* skipBtn_;
    QPushButton* startConsultBtn_;
    QPushButton* endConsultBtn_;
    QPushButton* pauseConsultBtn_;
    QPushButton* ultrasoundBtn_;
    
    QComboBox* doctorCombo_;
};

// =============================================================================
// 构造函数
// =============================================================================
DoctorWorkstation::DoctorWorkstation(QWidget* parent)
    : QMainWindow(parent)
    , networkClient_(nullptr)
    , currentDoctorId_(-1)
    , currentPatientId_(-1)
{
    setWindowTitle("智序医院 - 医生工作站");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // 创建网络客户端
    networkClient_ = new smartsched::client::NetworkClient(this);
    
    // 模拟医生数据
    currentDoctorId_ = 1;
    currentDoctorName_ = "张明华";
    
    setupUi();
    setupConnections();
    setupTrayIcon();
    
    // 启动时间更新
    timeTimer_ = new QTimer(this);
    connect(timeTimer_, &QTimer::timeout, this, [this]() {
        timeLabel_->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    timeTimer_->start(1000);
    
    // 自动连接
    onConnect();
}

DoctorWorkstation::~DoctorWorkstation() {
    if (networkClient_) {
        networkClient_->disconnectFromServer();
    }
}

// =============================================================================
// UI初始化
// =============================================================================
void DoctorWorkstation::setupUi() {
    // 设置字体
    QFont defaultFont("Microsoft YaHei, SimHei, sans-serif");
    QApplication::setFont(defaultFont);
    
    // ========== 菜单栏 ==========
    QMenuBar* menuBar = this->menuBar();
    
    QMenu* fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("刷新队列", this, &DoctorWorkstation::onRefreshQueue, QKeySequence::Refresh);
    fileMenu->addSeparator();
    fileMenu->addAction("导出报表", this, &DoctorWorkstation::onRefreshStatistics);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close, QKeySequence::Quit);
    
    QMenu* patientMenu = menuBar->addMenu("患者(&P)");
    patientMenu->addAction("开始接诊", this, &DoctorWorkstation::onStartConsultation, QKeySequence(tr("Ctrl+S")));
    patientMenu->addAction("结束接诊", this, &DoctorWorkstation::onEndConsultation, QKeySequence(tr("Ctrl+E")));
    patientMenu->addAction("暂停接诊", this, &DoctorWorkstation::onPauseConsultation);
    patientMenu->addSeparator();
    patientMenu->addAction("申请B超", this, &DoctorWorkstation::onRequestUltrasound);
    
    QMenu* settingsMenu = menuBar->addMenu("设置(&S)");
    settingsMenu->addAction("连接设置", this, &DoctorWorkstation::onSettings);
    settingsMenu->addSeparator();
    settingsMenu->addAction("关于", this, &DoctorWorkstation::onAbout);
    
    // ========== 工具栏 ==========
    QToolBar* toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);
    
    QAction* connectAction = toolBar->addAction("连接", this, &DoctorWorkstation::onConnect);
    connectAction->setObjectName("connectAction");
    
    toolBar->addSeparator();
    
    toolBar->addAction("刷新队列", this, &DoctorWorkstation::onRefreshQueue);
    
    toolBar->addSeparator();
    
    callNextBtn_ = toolBar->addAction("叫号", this, &DoctorWorkstation::onCallNext);
    callNextBtn_->setEnabled(false);
    
    callSpecificBtn_ = toolBar->addAction("指定叫号", this, &DoctorWorkstation::onCallSpecific);
    
    recallBtn_ = toolBar->addAction("重呼", this, &DoctorWorkstation::onRecall);
    recallBtn_->setEnabled(false);
    
    skipBtn_ = toolBar->addAction("跳过", this, &DoctorWorkstation::onSkip);
    skipBtn_->setEnabled(false);
    
    toolBar->addSeparator();
    
    startConsultBtn_ = toolBar->addAction("开始接诊", this, &DoctorWorkstation::onStartConsultation);
    startConsultBtn_->setEnabled(false);
    
    endConsultBtn_ = toolBar->addAction("结束接诊", this, &DoctorWorkstation::onEndConsultation);
    endConsultBtn_->setEnabled(false);
    
    pauseConsultBtn_ = toolBar->addAction("暂停", this, &DoctorWorkstation::onPauseConsultation);
    pauseConsultBtn_->setEnabled(false);
    
    ultrasoundBtn_ = toolBar->addAction("申请B超", this, &DoctorWorkstation::onRequestUltrasound);
    ultrasoundBtn_->setEnabled(false);
    
    // ========== 中央部件 ==========
    QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, this);
    
    // ----- 左侧：排队列表 -----
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    
    // 医生选择
    QGroupBox* doctorBox = new QGroupBox("当前医生");
    QHBoxLayout* doctorLayout = new QHBoxLayout(doctorBox);
    
    doctorCombo_ = new QComboBox();
    doctorCombo_->addItem("张明华 - 主任医师", 1);
    doctorCombo_->addItem("李秀英 - 副主任医师", 2);
    doctorCombo_->addItem("王建国 - 主治医师", 3);
    doctorLayout->addWidget(doctorCombo_);
    
    QLabel* deptLabel = new QLabel("科室: 内科");
    doctorLayout->addWidget(deptLabel);
    doctorLayout->addStretch();
    
    leftLayout->addWidget(doctorBox);
    
    // 排队列表
    QGroupBox* queueBox = new QGroupBox("待诊患者队列");
    QVBoxLayout* queueLayout = new QVBoxLayout(queueBox);
    
    queueTable_ = new QTableWidget();
    queueTable_->setColumnCount(5);
    queueTable_->setHorizontalHeaderLabels({"排队号", "患者姓名", "性别", "年龄", "等待时间"});
    queueTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queueTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    queueTable_->setAlternatingRowColors(true);
    queueTable_->verticalHeader()->setVisible(false);
    queueTable_->setColumnWidth(0, 120);
    queueTable_->setColumnWidth(1, 100);
    queueTable_->setColumnWidth(2, 60);
    queueTable_->setColumnWidth(3, 60);
    queueTable_->setColumnWidth(4, 100);
    
    // 模拟数据
    queueTable_->insertRow(0);
    queueTable_->setItem(0, 0, new QTableWidgetItem("D01-2407010001"));
    queueTable_->setItem(0, 1, new QTableWidgetItem("张三"));
    queueTable_->setItem(0, 2, new QTableWidgetItem("男"));
    queueTable_->setItem(0, 3, new QTableWidgetItem("45"));
    queueTable_->setItem(0, 4, new QTableWidgetItem("5分钟"));
    
    queueTable_->insertRow(1);
    queueTable_->setItem(1, 0, new QTableWidgetItem("D01-2407010002"));
    queueTable_->setItem(1, 1, new QTableWidgetItem("李四"));
    queueTable_->setItem(1, 2, new QTableWidgetItem("女"));
    queueTable_->setItem(1, 3, new QTableWidgetItem("32"));
    queueTable_->setItem(1, 4, new QTableWidgetItem("12分钟"));
    
    queueTable_->insertRow(2);
    queueTable_->setItem(2, 0, new QTableWidgetItem("D01-2407010003"));
    queueTable_->setItem(2, 1, new QTableWidgetItem("王五"));
    queueTable_->setItem(2, 2, new QTableWidgetItem("男"));
    queueTable_->setItem(2, 3, new QTableWidgetItem("58"));
    queueTable_->setItem(2, 4, new QTableWidgetItem("25分钟"));
    
    queueLayout->addWidget(queueTable_);
    leftLayout->addWidget(queueBox);
    
    // ----- 中间：当前患者信息 -----
    QWidget* centerWidget = new QWidget();
    QVBoxLayout* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(5, 5, 5, 5);
    
    // 当前患者
    currentPatientBox_ = new QGroupBox("当前接诊患者");
    QGridLayout* patientLayout = new QGridLayout(currentPatientBox_);
    
    patientLayout->addWidget(new QLabel("姓    名:"), 0, 0);
    patientNameLabel_ = new QLabel("-");
    patientNameLabel_->setStyleSheet("font-size: 18px; font-weight: bold; color: #1976D2;");
    patientLayout->addWidget(patientNameLabel_, 0, 1);
    
    patientLayout->addWidget(new QLabel("性    别:"), 0, 2);
    patientGenderLabel_ = new QLabel("-");
    patientLayout->addWidget(patientGenderLabel_, 0, 3);
    
    patientLayout->addWidget(new QLabel("年    龄:"), 1, 0);
    patientAgeLabel_ = new QLabel("-");
    patientLayout->addWidget(patientAgeLabel_, 1, 1);
    
    patientLayout->addWidget(new QLabel("联系电话:"), 1, 2);
    patientPhoneLabel_ = new QLabel("-");
    patientLayout->addWidget(patientPhoneLabel_, 1, 3);
    
    patientLayout->addWidget(new QLabel("排队号码:"), 2, 0);
    QLabel* queueNumLabel = new QLabel("-");
    queueNumLabel->setStyleSheet("font-weight: bold; color: #FF5722;");
    patientLayout->addWidget(queueNumLabel, 2, 1);
    
    patientLayout->addWidget(new QLabel("等候时长:"), 2, 2);
    QLabel* waitTimeLabel = new QLabel("-");
    patientLayout->addWidget(waitTimeLabel, 2, 3);
    
    patientLayout->setColumnStretch(0, 1);
    patientLayout->setColumnStretch(1, 2);
    patientLayout->setColumnStretch(2, 1);
    patientLayout->setColumnStretch(3, 2);
    
    centerLayout->addWidget(currentPatientBox_);
    
    // 病历信息
    QGroupBox* recordBox = new QGroupBox("病历记录");
    QVBoxLayout* recordLayout = new QVBoxLayout(recordBox);
    
    QTabWidget* recordTabs = new QTabWidget();
    
    // 历史记录页
    QTextEdit* historyEdit = new QTextEdit();
    historyEdit->setReadOnly(true);
    historyEdit->setPlaceholderText("暂无历史记录");
    recordTabs->addTab(historyEdit, "历史就诊");
    
    // 本次就诊页
    QTextEdit* currentRecordEdit = new QTextEdit();
    currentRecordEdit->setPlaceholderText("请输入本次就诊记录...");
    recordTabs->addTab(currentRecordEdit, "本次就诊");
    
    // 处方页
    QTextEdit* prescriptionEdit = new QTextEdit();
    prescriptionEdit->setPlaceholderText("请输入处方信息...");
    recordTabs->addTab(prescriptionEdit, "处方");
    
    recordLayout->addWidget(recordTabs);
    centerLayout->addWidget(recordBox);
    
    // ----- 右侧：统计信息 -----
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    
    // 今日统计
    statisticsBox_ = new QGroupBox("今日统计");
    QVBoxLayout* statsLayout = new QVBoxLayout(statisticsBox_);
    
    QLabel* title = new QLabel("<h3>📊 今日接诊统计</h3>");
    statsLayout->addWidget(title);
    
    statsLayout->addWidget(new QLabel("已接诊人数:"));
    todayCountLabel_ = new QLabel("0 人");
    todayCountLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    statsLayout->addWidget(todayCountLabel_);
    
    statsLayout->addWidget(new QLabel("平均等候时间:"));
    avgWaitTimeLabel_ = new QLabel("0 分钟");
    avgWaitTimeLabel_->setStyleSheet("font-size: 18px; color: #FF9800;");
    statsLayout->addWidget(avgWaitTimeLabel_);
    
    statsLayout->addWidget(new QLabel("当前队列人数:"));
    currentQueueLabel_ = new QLabel("3 人");
    currentQueueLabel_->setStyleSheet("font-size: 18px; color: #2196F3;");
    statsLayout->addWidget(currentQueueLabel_);
    
    statsLayout->addStretch();
    rightLayout->addWidget(statisticsBox_);
    
    // 诊室状态
    QGroupBox* roomBox = new QGroupBox("诊室状态");
    QVBoxLayout* roomLayout = new QVBoxLayout(roomBox);
    
    QRadioButton* availableBtn = new QRadioButton("🟢 可接诊");
    availableBtn->setChecked(true);
    roomLayout->addWidget(availableBtn);
    
    QRadioButton* busyBtn = new QRadioButton("🟡 忙碌中");
    roomLayout->addWidget(busyBtn);
    
    QRadioButton* breakBtn = new QRadioButton("🟠 休息中");
    roomLayout->addWidget(breakBtn);
    
    roomLayout->addStretch();
    rightLayout->addWidget(roomBox);
    
    // B超预约状态
    QGroupBox* ultrasoundBox = new QGroupBox("B超室状态");
    QVBoxLayout* ultraLayout = new QVBoxLayout(ultrasoundBox);
    
    ultraLayout->addWidget(new QLabel("1号机: 🟢 空闲"));
    ultraLayout->addWidget(new QLabel("2号机: 🟡 检查中 (张三)"));
    ultraLayout->addWidget(new QLabel("3号机: 🟢 空闲"));
    
    ultraLayout->addStretch();
    rightLayout->addWidget(ultrasoundBox);
    
    // 添加到主分割器
    centralSplitter->addWidget(leftWidget);
    centralSplitter->addWidget(centerWidget);
    centralSplitter->addWidget(rightWidget);
    centralSplitter->setStretchFactor(0, 3);
    centralSplitter->setStretchFactor(1, 4);
    centralSplitter->setStretchFactor(2, 2);
    
    setCentralWidget(centralSplitter);
    
    // ========== 状态栏 ==========
    statusBar()->showMessage("就绪");
    
    QLabel* spacer = new QLabel();
    statusBar()->addPermanentWidget(spacer, 1);
    
    connectionLabel_ = new QLabel("⚫ 未连接");
    statusBar()->addPermanentWidget(connectionLabel_);
    
    statusBar()->addPermanentWidget(new QLabel("  |  "));
    
    statusLabel_ = new QLabel("医生: " + currentDoctorName_);
    statusBar()->addPermanentWidget(statusLabel_);
    
    statusBar()->addPermanentWidget(new QLabel("  |  "));
    
    timeLabel_ = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    statusBar()->addPermanentWidget(timeLabel_);
}

// =============================================================================
// 信号连接
// =============================================================================
void DoctorWorkstation::setupConnections() {
    // 连接状态变化
    connect(networkClient_, &smartsched::client::NetworkClient::stateChanged,
            this, &DoctorWorkstation::onConnectionStateChanged);
    
    // 消息接收
    connect(networkClient_, &smartsched::client::NetworkClient::messageReceived,
            this, &DoctorWorkstation::onMessageReceived);
}

// =============================================================================
// 托盘图标
// =============================================================================
void DoctorWorkstation::setupTrayIcon() {
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip("医生工作站");
    
    // 创建托盘菜单
    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction("显示窗口", this, &QWidget::showNormal);
    trayMenu->addSeparator();
    trayMenu->addAction("退出", qApp, &QApplication::quit);
    
    trayIcon->setContextMenu(trayMenu);
    
    // 点击托盘图标显示窗口
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::Reason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showNormal();
            activateWindow();
        }
    });
    
    trayIcon->show();
}

// =============================================================================
// 连接相关
// =============================================================================
void DoctorWorkstation::onConnect() {
    networkClient_->connectToServer();
    statusBar()->showMessage("正在连接服务器...", 3000);
}

void DoctorWorkstation::onDisconnect() {
    networkClient_->disconnectFromServer();
}

void DoctorWorkstation::onConnectionStateChanged(int state) {
    bool connected = (state == 3); // Connected状态
    updateConnectionStatus(connected);
    
    if (connected) {
        showNotification("连接成功", "已成功连接到服务器");
        statusBar()->showMessage("已连接", 3000);
    } else {
        statusBar()->showMessage("连接断开", 3000);
    }
}

void DoctorWorkstation::updateConnectionStatus(bool connected) {
    connectionLabel_->setText(connected ? "🟢 已连接" : "⚫ 未连接");
    
    // 更新工具栏按钮状态
    QToolBar* toolBar = findChild<QToolBar*>("mainToolBar");
    
    // 根据连接状态启用/禁用按钮
    callNextBtn_->setEnabled(connected);
}

// =============================================================================
// 叫号相关
// =============================================================================
void DoctorWorkstation::onCallNext() {
    // 获取队首患者
    if (queueTable_->rowCount() > 0) {
        QString queueNumber = queueTable_->item(0, 0)->text();
        QString patientName = queueTable_->item(0, 1)->text();
        
        // 显示叫号提示
        QMessageBox::information(this, "叫号", 
            QString("正在呼叫: %1\n排队号: %2").arg(patientName).arg(queueNumber));
        
        // 更新按钮状态
        startConsultBtn_->setEnabled(true);
        recallBtn_->setEnabled(true);
        skipBtn_->setEnabled(true);
    }
}

void DoctorWorkstation::onCallSpecific() {
    bool ok;
    QString queueNumber = QInputDialog::getText(this, "指定叫号", 
        "请输入排队号:", QLineEdit::Normal, "", &ok);
    
    if (ok && !queueNumber.isEmpty()) {
        QMessageBox::information(this, "叫号", 
            QString("正在呼叫: %1").arg(queueNumber));
    }
}

void DoctorWorkstation::onRecall() {
    if (queueTable_->rowCount() > 0) {
        QString patientName = queueTable_->item(0, 1)->text();
        QMessageBox::information(this, "重呼", 
            QString("正在重呼: %1").arg(patientName));
    }
}

void DoctorWorkstation::onSkip() {
    int currentRow = queueTable_->currentRow();
    if (currentRow >= 0) {
        // 移动到队列末尾
        QMessageBox::information(this, "跳过", "患者已移至队列末尾");
    }
}

// =============================================================================
// 接诊相关
// =============================================================================
void DoctorWorkstation::onStartConsultation() {
    if (queueTable_->rowCount() > 0) {
        QString patientName = queueTable_->item(0, 1)->text();
        
        // 更新当前患者信息
        patientNameLabel_->setText(patientName);
        patientGenderLabel_->setText(queueTable_->item(0, 2)->text());
        patientAgeLabel_->setText(queueTable_->item(0, 3)->text());
        
        // 更新按钮状态
        startConsultBtn_->setEnabled(false);
        endConsultBtn_->setEnabled(true);
        pauseConsultBtn_->setEnabled(true);
        ultrasoundBtn_->setEnabled(true);
        
        statusBar()->showMessage(QString("正在接诊: %1").arg(patientName));
    }
}

void DoctorWorkstation::onEndConsultation() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "结束接诊",
        "确定要结束当前接诊吗?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 从队列移除
        queueTable_->removeRow(0);
        
        // 重置当前患者
        patientNameLabel_->setText("-");
        patientGenderLabel_->setText("-");
        patientAgeLabel_->setText("-");
        patientPhoneLabel_->setText("-");
        
        // 更新统计
        int count = todayCountLabel_->text().remove(" 人").toInt() + 1;
        todayCountLabel_->setText(QString("%1 人").arg(count));
        
        // 更新按钮状态
        startConsultBtn_->setEnabled(true);
        endConsultBtn_->setEnabled(false);
        pauseConsultBtn_->setEnabled(false);
        ultrasoundBtn_->setEnabled(false);
        
        statusBar()->showMessage("接诊已结束");
    }
}

void DoctorWorkstation::onPauseConsultation() {
    QMessageBox::information(this, "暂停", "接诊已暂停，患者将保留在队列中");
    
    // 更新按钮状态
    startConsultBtn_->setEnabled(true);
    pauseConsultBtn_->setEnabled(false);
}

// =============================================================================
// B超相关
// =============================================================================
void DoctorWorkstation::onRequestUltrasound() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "申请B超",
        "确定要为此患者申请B超检查吗?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, "B超预约", 
            "B超检查已预约，请告知患者前往B超室候诊");
    }
}

// =============================================================================
// 刷新
// =============================================================================
void DoctorWorkstation::onRefreshQueue() {
    statusBar()->showMessage("正在刷新队列...", 2000);
    
    // 模拟更新等待时间
    for (int i = 0; i < queueTable_->rowCount(); ++i) {
        int minutes = queueTable_->item(i, 4)->text().remove("分钟").toInt() + 1;
        queueTable_->item(i, 4)->setText(QString("%1分钟").arg(minutes));
    }
    
    currentQueueLabel_->setText(QString("%1 人").arg(queueTable_->rowCount()));
}

void DoctorWorkstation::onRefreshStatistics() {
    // 导出报表
    QString fileName = QFileDialog::getSaveFileName(this, "导出报表",
        QString("接诊统计_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV文件 (*.csv)");
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, "导出成功", 
            QString("报表已导出至: %1").arg(fileName));
    }
}

// =============================================================================
// 设置
// =============================================================================
void DoctorWorkstation::onSettings() {
    QMessageBox::information(this, "连接设置", "连接设置对话框（待实现）");
}

void DoctorWorkstation::onAbout() {
    QMessageBox::about(this, "关于",
        QString("<h3>智序医院 - 医生工作站</h3>"
                "<p>版本: %1</p>"
                "<p>© 2024-2026 SmartSched Healthcare Technology</p>").arg(SMARTSCHED_VERSION_STRING));
}

// =============================================================================
// 网络消息
// =============================================================================
void DoctorWorkstation::onMessageReceived(const QVariant& json) {
    // 处理接收到的消息
    qDebug() << "Received message:" << json;
}

// =============================================================================
// 通知
// =============================================================================
void DoctorWorkstation::showNotification(const QString& title, const QString& message) {
    QSystemTrayIcon* tray = findChild<QSystemTrayIcon*>();
    if (tray) {
        tray->showMessage(title, message, QSystemTrayIcon::Information, 3000);
    }
}

// =============================================================================
// 关闭事件
// =============================================================================
void DoctorWorkstation::closeEvent(QCloseEvent* event) {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认退出",
        "确定要退出医生工作站吗?",
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
    
    // 应用信息
    app.setApplicationName("SmartSched-医生工作站");
    app.setApplicationVersion(SMARTSCHED_VERSION_STRING);
    app.setOrganizationName("SmartSched Healthcare");
    app.setQuitOnLastWindowClosed(false);
    
    // 创建主窗口
    DoctorWorkstation window;
    window.show();
    
    return app.exec();
}

#include "doctormain.moc"
