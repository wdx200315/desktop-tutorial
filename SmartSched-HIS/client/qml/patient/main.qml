// =============================================================================
// SmartSched-HIS 患者挂号终端 - 主界面
// 功能：触摸式挂号界面
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQuick.Window 2.15

ApplicationWindow {
    id: mainWindow
    title: HOSPITAL_NAME + " - 自助挂号终端"
    visible: true
    width: 1024
    height: 768
    maximumWidth: 1920
    maximumHeight: 1080
    minimumWidth: 800
    minimumHeight: 600
    
    // 全屏模式（触摸终端常用）
    // visibility: Window.FullScreen
    
    // 主题颜色
    readonly property color primaryColor: "#1976D2"
    readonly property color primaryDark: "#1565C0"
    readonly property color accentColor: "#FF5722"
    readonly property color backgroundColor: "#FAFAFA"
    readonly property color successColor: "#4CAF50"
    
    // 页面栈
    property StackView stackView: null
    
    // 连接状态
    property bool isConnected: networkClient && networkClient.state === 3
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: backgroundColor
    }
    
    // 顶部栏
    header: ToolBar {
        height: 70
        background: Rectangle {
            color: primaryColor
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            // 医院Logo/名称
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 2
                
                Text {
                    text: HOSPITAL_NAME
                    font.pixelSize: 28
                    font.bold: true
                    color: "#FFFFFF"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
                
                Text {
                    text: "门诊自助挂号系统"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                    opacity: 0.9
                }
            }
            
            Item { Layout.fillWidth: true }
            
            // 连接状态指示器
            RowLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: isConnected ? "#4CAF50" : "#F44336"
                }
                
                Text {
                    text: isConnected ? "在线" : "离线"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                }
            }
            
            // 时间显示
            Text {
                id: timeDisplay
                font.pixelSize: 24
                font.bold: true
                color: "#FFFFFF"
                font.family: "Microsoft YaHei, sans-serif"
                
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        var now = new Date()
                        timeDisplay.text = Qt.formatDateTime(now, "HH:mm:ss")
                    }
                }
            }
        }
    }
    
    // 页面内容区
    StackView {
        id: pageStack
        anchors.fill: parent
        anchors.top: parent.header
        anchors.bottom: parent.footer
        
        initialItem: homePage
    }
    
    // 底部栏
    footer: ToolBar {
        height: 50
        background: Rectangle {
            color: "#EEEEEE"
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            Text {
                text: "版本: " + APP_VERSION
                font.pixelSize: 12
                color: "#757575"
            }
            
            Item { Layout.fillWidth: true }
            
            Text {
                text: "如有疑问，请联系导诊台工作人员"
                font.pixelSize: 12
                color: "#757575"
            }
        }
    }
    
    // =====================================================================
    // 首页 - 主菜单
    // =====================================================================
    Component {
        id: homePage
        
        Rectangle {
            color: backgroundColor
            
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 30
                
                // 标题
                Text {
                    text: "请选择服务"
                    font.pixelSize: 36
                    font.bold: true
                    color: "#212121"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                // 功能按钮网格
                GridLayout {
                    columns: 3
                    columnSpacing: 40
                    rowSpacing: 30
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    // 按钮1: 在线挂号
                    homeButton {
                        iconSource: "qrc:/icons/register.png"
                        title: "在线挂号"
                        subtitle: "选择科室和医生"
                        color: primaryColor
                        onClicked: {
                            pageStack.push("qrc:/qml/patient/DepartmentList.qml")
                        }
                    }
                    
                    // 按钮2: 排队查询
                    homeButton {
                        iconSource: "qrc:/icons/queue.png"
                        title: "排队查询"
                        subtitle: "查看当前排队状态"
                        color: "#4CAF50"
                        onClicked: {
                            pageStack.push("qrc:/qml/patient/QueueStatusPage.qml")
                        }
                    }
                    
                    // 按钮3: 打印凭条
                    homeButton {
                        iconSource: "qrc:/icons/print.png"
                        title: "补打凭条"
                        subtitle: "补打挂号凭条"
                        color: "#FF9800"
                        onClicked: {
                            showMessage("功能开发中")
                        }
                    }
                }
                
                // 温馨提示
                Rectangle {
                    width: 600
                    height: 80
                    radius: 10
                    color: "#E3F2FD"
                    border.width: 1
                    border.color: "#BBDEFB"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        
                        Text {
                            text: "📋 温馨提示"
                            font.pixelSize: 16
                            font.bold: true
                            color: primaryColor
                        }
                        
                        Text {
                            text: "• 请凭挂号凭条到相应诊区候诊\n• 听到呼叫后请进入诊室就诊"
                            font.pixelSize: 14
                            color: "#424242"
                            font.family: "Microsoft YaHei, sans-serif"
                        }
                    }
                }
            }
        }
    }
    
    // =====================================================================
    // 自定义首页按钮组件
    // =====================================================================
    component homeButton: Rectangle {
        property string iconSource
        property string title
        property string subtitle
        property color color
        signal clicked()
        
        width: 220
        height: 200
        radius: 16
        color: color
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            
            Item { Layout.fillHeight: true }
            
            // 图标
            Rectangle {
                width: 80
                height: 80
                radius: 40
                color: "#FFFFFF"
                opacity: 0.2
                Layout.alignment: Qt.AlignHCenter
                
                Image {
                    source: iconSource
                    width: 48
                    height: 48
                    anchors.centerIn: parent
                    fillMode: Image.PreserveAspectFit
                    visible: source !== ""
                }
                
                // 备用图标（纯色圆）
                Text {
                    text: iconSource === "qrc:/icons/register.png" ? "📝" :
                          iconSource === "qrc:/icons/queue.png" ? "📋" : "🖨️"
                    font.pixelSize: 40
                    anchors.centerIn: parent
                    visible: source === "" || !parent.parent.parent.visible
                }
            }
            
            Text {
                text: title
                font.pixelSize: 22
                font.bold: true
                color: "#FFFFFF"
                font.family: "Microsoft YaHei, SimHei, sans-serif"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Text {
                text: subtitle
                font.pixelSize: 14
                color: "#FFFFFF"
                opacity: 0.9
                font.family: "Microsoft YaHei, sans-serif"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Item { Layout.fillHeight: true }
        }
        
        MouseArea {
            anchors.fill: parent
            onClicked: parent.clicked()
        }
        
        // 点击效果
        states: State {
            name: "pressed"
            when: parentMouseArea.pressed
            PropertyChanges {
                target: parent
                scale: 0.95
                opacity: 0.8
            }
        }
        
        property MouseArea parentMouseArea: MouseArea { anchors.fill: parent; onClicked: parent.clicked() }
        
        transform: Scale {
            id: scaleTransform
            origin.x: parent.width / 2
            origin.y: parent.height / 2
        }
        
        transitions: Transition {
            NumberAnimation { property: "scale"; duration: 100 }
            NumberAnimation { property: "opacity"; duration: 100 }
        }
    }
    
    // =====================================================================
    // 辅助函数
    // =====================================================================
    function showMessage(message, duration) {
        snackbar.text = message
        snackbar.open()
        if (duration === undefined) duration = 3000
        messageTimer.interval = duration
        messageTimer.restart()
    }
    
    // 消息提示条
    Snackbar {
        id: snackbar
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        anchors.horizontalCenter: parent.horizontalCenter
    }
    
    Timer {
        id: messageTimer
        onTriggered: snackbar.close()
    }
    
    // 确认对话框
    Dialog {
        id: confirmDialog
        title: "确认"
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        
        Label {
            text: confirmDialog.text
            font.pixelSize: 16
        }
    }
    
    // 加载指示器
    Popup {
        id: loadingPopup
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.NoAutoClose
        
        ColumnLayout {
            spacing: 20
            
            ProgressBar {
                indeterminate: true
                Layout.preferredWidth: 200
            }
            
            Text {
                text: "加载中..."
                font.pixelSize: 14
                color: "#757575"
            }
        }
    }
}
