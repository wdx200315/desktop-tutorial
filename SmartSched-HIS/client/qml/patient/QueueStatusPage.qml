// =============================================================================
// SmartSched-HIS 排队状态查询页面
// 功能：查询当前排队状态和预计等待时间
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Page {
    id: queueStatusPage
    title: "排队状态查询"
    
    // 背景
    Rectangle {
        anchors.fill: parent
        color: "#FAFAFA"
    }
    
    // 顶部信息栏
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "#FFFFFF"
        border.width: 1
        border.color: "#E0E0E0"
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            // 返回按钮
            ToolButton {
                text: "←"
                font.pixelSize: 24
                onClicked: pageStack.pop()
            }
            
            Text {
                text: "排队状态查询"
                font.pixelSize: 20
                font.bold: true
                color: "#212121"
                font.family: "Microsoft YaHei, SimHei, sans-serif"
            }
            
            Item { Layout.fillWidth: true }
            
            // 刷新按钮
            ToolButton {
                text: "🔄"
                font.pixelSize: 20
                onClicked: refreshStatus()
            }
        }
    }
    
    // 内容区
    ColumnLayout {
        anchors.top: headerBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 20
        
        // 输入区域
        Rectangle {
            Layout.fillWidth: true
            height: 100
            radius: 12
            color: "#FFFFFF"
            border.width: 1
            border.color: "#E0E0E0"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15
                
                Text {
                    text: "请输入您的排队号或手机号"
                    font.pixelSize: 14
                    color: "#757575"
                }
                
                RowLayout {
                    spacing: 15
                    
                    TextField {
                        id: queryInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 45
                        placeholderText: "例如：D01-2407010001 或 13800138001"
                        font.pixelSize: 16
                        font.family: "Microsoft YaHei, sans-serif"
                        
                        // 回车查询
                        onAccepted: queryQueueStatus()
                    }
                    
                    Button {
                        text: "查询"
                        font.pixelSize: 16
                        font.bold: true
                        buttonColor: "#1976D2"
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 45
                        
                        onClicked: queryQueueStatus()
                    }
                }
            }
        }
        
        // 状态显示区
        Rectangle {
            id: statusCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#FFFFFF"
            border.width: 1
            border.color: "#E0E0E0"
            visible: false
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 25
                spacing: 20
                
                // 状态标题
                RowLayout {
                    spacing: 15
                    
                    // 状态图标
                    Rectangle {
                        width: 60
                        height: 60
                        radius: 30
                        color: "#E3F2FD"
                        
                        Text {
                            text: "📋"
                            font.pixelSize: 32
                            anchors.centerIn: parent
                        }
                    }
                    
                    ColumnLayout {
                        spacing: 4
                        
                        Text {
                            text: queryResult.queueNumber
                            font.pixelSize: 28
                            font.bold: true
                            color: "#1976D2"
                            font.family: "Microsoft YaHei, sans-serif"
                        }
                        
                        Text {
                            text: "您的排队号"
                            font.pixelSize: 14
                            color: "#757575"
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    // 状态标签
                    Rectangle {
                        width: 80
                        height: 36
                        radius: 18
                        color: getStatusColor(queryResult.status)
                        
                        Text {
                            text: getStatusText(queryResult.status)
                            font.pixelSize: 14
                            font.bold: true
                            color: "#FFFFFF"
                            anchors.centerIn: parent
                        }
                    }
                }
                
                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#E0E0E0"
                }
                
                // 详细信息网格
                GridLayout {
                    columns: 2
                    rowSpacing: 20
                    columnSpacing: 40
                    Layout.fillWidth: true
                    
                    // 科室
                    InfoItem {
                        label: "科室"
                        value: queryResult.deptName
                        icon: "🏥"
                    }
                    
                    // 医生
                    InfoItem {
                        label: "医生"
                        value: queryResult.doctorName
                        icon: "👨‍⚕️"
                    }
                    
                    // 当前位置
                    InfoItem {
                        label: "当前排队位置"
                        value: "第 " + queryResult.position + " 位"
                        icon: "📍"
                        highlight: true
                    }
                    
                    // 预计等待
                    InfoItem {
                        label: "预计等待时间"
                        value: queryResult.estimatedWait
                        icon: "⏱️"
                        highlight: true
                    }
                }
                
                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#E0E0E0"
                }
                
                // 提示信息
                Rectangle {
                    Layout.fillWidth: true
                    height: 70
                    radius: 8
                    color: "#FFF8E1"
                    border.width: 1
                    border.color: "#FFE082"
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 15
                        
                        Text {
                            text: "💡"
                            font.pixelSize: 24
                        }
                        
                        Text {
                            text: "温馨提示\n请注意查看候诊区显示屏，听到呼叫后进入诊室就诊"
                            font.pixelSize: 13
                            color: "#795548"
                            font.family: "Microsoft YaHei, sans-serif"
                        }
                    }
                }
                
                Item { Layout.fillHeight: true }
                
                // 操作按钮
                RowLayout {
                    spacing: 15
                    Layout.alignment: Qt.AlignHCenter
                    
                    Button {
                        text: "取消挂号"
                        font.pixelSize: 14
                        buttonColor: "#F44336"
                        isOutlined: true
                        Layout.preferredWidth: 120
                        
                        onClicked: {
                            confirmCancel()
                        }
                    }
                    
                    Button {
                        text: "返回首页"
                        font.pixelSize: 14
                        buttonColor: "#1976D2"
                        Layout.preferredWidth: 120
                        
                        onClicked: pageStack.pop()
                    }
                }
            }
        }
        
        // 空状态提示
        Rectangle {
            id: emptyState
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#FFFFFF"
            visible: true
            
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 15
                
                Text {
                    text: "🔍"
                    font.pixelSize: 60
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "请输入排队号或手机号查询"
                    font.pixelSize: 16
                    color: "#757575"
                    font.family: "Microsoft YaHei, sans-serif"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
    
    // =====================================================================
    // 辅助组件
    // =====================================================================
    component InfoItem: ColumnLayout {
        property string label
        property string value
        property string icon
        property bool highlight: false
        
        spacing: 6
        
        RowLayout {
            spacing: 8
            
            Text {
                text: icon
                font.pixelSize: 18
            }
            
            Text {
                text: label
                font.pixelSize: 13
                color: "#757575"
            }
        }
        
        Text {
            text: value
            font.pixelSize: highlight ? 20 : 16
            font.bold: true
            color: highlight ? "#FF5722" : "#212121"
            font.family: "Microsoft YaHei, SimHei, sans-serif"
        }
    }
    
    // =====================================================================
    // 数据和函数
    // =====================================================================
    property var queryResult: ({
        "queueNumber": "D01-2407010001",
        "status": 0,
        "deptName": "内科",
        "doctorName": "张明华",
        "position": 5,
        "estimatedWait": "约 15 分钟"
    })
    
    function queryQueueStatus() {
        var input = queryInput.text.trim()
        if (input === "") {
            return
        }
        
        // 显示状态卡片
        statusCard.visible = true
        emptyState.visible = false
        
        // 模拟查询结果
        queryResult = {
            "queueNumber": input.startsWith("D") ? input : "D01-2407010001",
            "status": 0,
            "deptName": "内科",
            "doctorName": "张明华",
            "position": Math.floor(Math.random() * 20) + 1,
            "estimatedWait": "约 " + (Math.floor(Math.random() * 30) + 5) + " 分钟"
        }
    }
    
    function refreshStatus() {
        if (statusCard.visible) {
            queryQueueStatus()
        }
    }
    
    function getStatusColor(status) {
        switch(status) {
            case 0: return "#FFC107" // 等待中
            case 1: return "#2196F3" // 诊治中
            case 2: return "#4CAF50" // 已完成
            case 3: return "#9E9E9E" // 已取消
            default: return "#757575"
        }
    }
    
    function getStatusText(status) {
        switch(status) {
            case 0: return "等待中"
            case 1: return "诊治中"
            case 2: return "已完成"
            case 3: return "已取消"
            default: return "未知"
        }
    }
    
    function confirmCancel() {
        // 显示取消确认对话框
        cancelDialog.open()
    }
    
    // 取消对话框
    Dialog {
        id: cancelDialog
        title: "确认取消挂号"
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        
        ColumnLayout {
            spacing: 15
            
            Text {
                text: "确定要取消挂号吗？"
                font.pixelSize: 16
                color: "#212121"
            }
            
            Text {
                text: "取消后需要重新排队"
                font.pixelSize: 14
                color: "#F44336"
            }
        }
        
        onAccepted: {
            // 执行取消
            pageStack.pop()
        }
    }
    
    // 定时刷新
    Timer {
        interval: 30000 // 30秒
        running: statusCard.visible
        repeat: true
        onTriggered: refreshStatus()
    }
}
