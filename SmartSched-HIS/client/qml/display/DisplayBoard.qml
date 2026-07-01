// =============================================================================
// SmartSched-HIS 排队看板大屏
// 功能：显示各科室实时排队状态，滚动播放叫号信息
// =============================================================================

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtQuick.Window 2.15
import QtQuick.Controls.Material 2.15

ApplicationWindow {
    id: mainWindow
    title: HOSPITAL_NAME + " - 排队叫号显示系统"
    visible: true
    width: 1920
    height: 1080
    // 全屏模式
    visibility: Window.FullScreen
    
    // 主题颜色
    readonly property color primaryColor: "#1976D2"
    readonly property color accentColor: "#FF5722"
    readonly property color successColor: "#4CAF50"
    readonly property color warningColor: "#FFC107"
    
    // 背景渐变
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1a237e" }
            GradientStop { position: 1.0; color: "#0d47a1" }
        }
    }
    
    // ========== 顶部标题栏 ==========
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 100
        color: "transparent"
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 40
            anchors.rightMargin: 40
            
            // 医院Logo
            ColumnLayout {
                spacing: 5
                
                Text {
                    text: "🏥"
                    font.pixelSize: 50
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            
            // 标题
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignVCenter
                
                Text {
                    text: HOSPITAL_NAME
                    font.pixelSize: 42
                    font.bold: true
                    color: "#FFFFFF"
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
                
                Text {
                    text: "门诊排队叫号系统"
                    font.pixelSize: 20
                    color: "#FFFFFF"
                    opacity: 0.9
                }
            }
            
            Item { Layout.fillWidth: true }
            
            // 时间显示
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignVCenter
                
                Text {
                    id: currentTime
                    text: new Date().toLocaleTimeString("zh-CN", Locale.ShortFormat)
                    font.pixelSize: 48
                    font.bold: true
                    color: "#FFD54F"
                    font.family: "Microsoft YaHei, sans-serif"
                    
                    Timer {
                        interval: 1000
                        running: true
                        repeat: true
                        onTriggered: {
                            currentTime.text = new Date().toLocaleTimeString("zh-CN", Locale.ShortFormat)
                        }
                    }
                }
                
                Text {
                    text: new Date().toLocaleDateString("zh-CN", Locale.ShortFormat)
                    font.pixelSize: 20
                    color: "#FFFFFF"
                    opacity: 0.8
                }
            }
            
            // 当前日期
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignVCenter
                
                Text {
                    text: Qt.formatDate(new Date(), "yyyy年MM月dd日")
                    font.pixelSize: 24
                    font.bold: true
                    color: "#FFFFFF"
                    font.family: "Microsoft YaHei, sans-serif"
                }
                
                Text {
                    text: new Date().toLocaleString("zh-CN", Locale.LongFormat).split(" ").slice(-1)[0]
                    font.pixelSize: 18
                    color: "#FFFFFF"
                    opacity: 0.8
                }
            }
        }
    }
    
    // ========== 中央叫号区域 ==========
    Rectangle {
        id: callArea
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: queueListArea.top
        anchors.margins: 30
        radius: 20
        color: "#FFFFFF"
        
        // 阴影效果
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 22
            color: "#00000033"
            z: -1
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 20
            
            // 当前叫号标题
            Rectangle {
                Layout.fillWidth: true
                height: 60
                radius: 10
                color: "#E3F2FD"
                
                Text {
                    text: "📢 正在呼叫"
                    font.pixelSize: 24
                    font.bold: true
                    color: primaryColor
                    anchors.centerIn: parent
                    font.family: "Microsoft YaHei, SimHei, sans-serif"
                }
            }
            
            // 叫号卡片
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 40
                
                // 左侧：主叫号显示
                Rectangle {
                    Layout.fillWidth: 2
                    Layout.fillHeight: true
                    radius: 15
                    color: "#FFF8E1"
                    border.width: 3
                    border.color: accentColor
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 15
                        
                        // 排队号
                        Text {
                            text: "排队号"
                            font.pixelSize: 20
                            color: "#757575"
                            font.family: "Microsoft YaHei, sans-serif"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            id: currentQueueNumber
                            text: "D01-0012"
                            font.pixelSize: 72
                            font.bold: true
                            color: accentColor
                            font.family: "Microsoft YaHei, sans-serif"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        // 患者姓名
                        Text {
                            id: currentPatientName
                            text: "张 三"
                            font.pixelSize: 56
                            font.bold: true
                            color: "#212121"
                            font.family: "Microsoft YaHei, SimHei, sans-serif"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Item { Layout.fillHeight: true }
                        
                        // 科室和诊室
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 30
                            
                            Rectangle {
                                padding: 10
                                radius: 8
                                color: "#E3F2FD"
                                
                                Text {
                                    text: "🏥 内科"
                                    font.pixelSize: 18
                                    color: primaryColor
                                }
                            }
                            
                            Rectangle {
                                padding: 10
                                radius: 8
                                color: "#E8F5E9"
                                
                                Text {
                                    text: "🚪 3号诊室"
                                    font.pixelSize: 18
                                    color: successColor
                                }
                            }
                        }
                        
                        // 请入室提示
                        Rectangle {
                            Layout.fillWidth: true
                            height: 70
                            radius: 10
                            color: accentColor
                            
                            Text {
                                text: "请进入诊室就诊"
                                font.pixelSize: 32
                                font.bold: true
                                color: "#FFFFFF"
                                anchors.centerIn: parent
                                font.family: "Microsoft YaHei, SimHei, sans-serif"
                            }
                        }
                    }
                }
                
                // 右侧：叫号记录
                Rectangle {
                    Layout.fillWidth: 1
                    Layout.fillHeight: true
                    radius: 15
                    color: "#F5F5F5"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        Text {
                            text: "最近叫号"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#424242"
                            font.family: "Microsoft YaHei, sans-serif"
                        }
                        
                        // 叫号历史列表
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: ListModel {
                                id: callHistoryModel
                            }
                            
                            delegate: Rectangle {
                                width: parent ? parent.width : 0
                                height: 60
                                radius: 8
                                color: index % 2 === 0 ? "#FFFFFF" : "#F5F5F5"
                                
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    
                                    Text {
                                        text: queueNum
                                        font.pixelSize: 18
                                        font.bold: true
                                        color: primaryColor
                                        Layout.preferredWidth: 120
                                    }
                                    
                                    Text {
                                        text: patient
                                        font.pixelSize: 16
                                        color: "#212121"
                                        Layout.preferredWidth: 80
                                    }
                                    
                                    Text {
                                        text: dept
                                        font.pixelSize: 14
                                        color: "#757575"
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                    
                                    Text {
                                        text: time
                                        font.pixelSize: 14
                                        color: "#9E9E9E"
                                    }
                                }
                            }
                        }
                    }
                    
                    Component.onCompleted: {
                        // 模拟数据
                        callHistoryModel.append({"queueNum": "D01-0011", "patient": "李四", "dept": "内科", "time": "10:32"})
                        callHistoryModel.append({"queueNum": "D02-0008", "patient": "王五", "dept": "外科", "time": "10:28"})
                        callHistoryModel.append({"queueNum": "D03-0015", "patient": "赵六", "dept": "儿科", "time": "10:25"})
                        callHistoryModel.append({"queueNum": "D01-0010", "patient": "孙七", "dept": "内科", "time": "10:20"})
                    }
                }
            }
        }
    }
    
    // ========== 底部排队列表 ==========
    Rectangle {
        id: queueListArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 280
        anchors.margins: 30
        anchors.bottomMargin: 20
        radius: 15
        color: "#FFFFFF"
        
        // 阴影
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 17
            color: "#00000022"
            z: -1
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            // 各科室排队情况
            Repeater {
                model: ListModel {
                    id: deptQueueModel
                }
                
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#FAFAFA"
                    border.width: 1
                    border.color: "#E0E0E0"
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5
                        
                        // 科室名称
                        Text {
                            text: deptName
                            font.pixelSize: 18
                            font.bold: true
                            color: "#424242"
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.family: "Microsoft YaHei, sans-serif"
                        }
                        
                        // 排队人数
                        Text {
                            text: queueCount
                            font.pixelSize: 48
                            font.bold: true
                            color: queueCount > 20 ? "#F44336" : (queueCount > 10 ? "#FF9800" : successColor)
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "人排队"
                            font.pixelSize: 14
                            color: "#757575"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#E0E0E0"
                        }
                        
                        // 下一位
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 10
                            
                            Text {
                                text: "下一位:"
                                font.pixelSize: 12
                                color: "#9E9E9E"
                            }
                            
                            Text {
                                text: nextQueue
                                font.pixelSize: 14
                                font.bold: true
                                color: primaryColor
                            }
                        }
                    }
                }
                
                Component.onCompleted: {
                    // 模拟各科室数据
                    deptQueueModel.append({"deptName": "内科", "queueCount": 8, "nextQueue": "D01-0013"})
                    deptQueueModel.append({"deptName": "外科", "queueCount": 5, "nextQueue": "D02-0009"})
                    deptQueueModel.append({"deptName": "儿科", "queueCount": 15, "nextQueue": "D03-0016"})
                    deptQueueModel.append({"deptName": "妇科", "queueCount": 12, "nextQueue": "D04-0013"})
                    deptQueueModel.append({"deptName": "骨科", "queueCount": 6, "nextQueue": "D05-0007"})
                }
            }
        }
    }
    
    // ========== B超室状态 ==========
    Rectangle {
        id: ultrasoundArea
        anchors.top: header.bottom
        anchors.right: parent.right
        anchors.bottom: callArea.top
        anchors.margins: 30
        anchors.bottomMargin: 0
        anchors.topMargin: 0
        width: 200
        radius: 15
        color: "#FFFFFF"
        visible: false // 默认隐藏
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10
            
            Text {
                text: "🔬 B超室"
                font.pixelSize: 16
                font.bold: true
                color: "#424242"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            // 机器状态
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 6
                color: "#E8F5E9"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    
                    Text { text: "1号机:"; font.pixelSize: 12; color: "#757575" }
                    Text { text: "🟢 空闲"; font.pixelSize: 12; color: successColor }
                }
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 6
                color: "#FFF3E0"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    
                    Text { text: "2号机:"; font.pixelSize: 12; color: "#757575" }
                    Text { text: "🟡 检查中"; font.pixelSize: 12; color: "#FF9800" }
                }
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 6
                color: "#E8F5E9"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    
                    Text { text: "3号机:"; font.pixelSize: 12; color: "#757575" }
                    Text { text: "🟢 空闲"; font.pixelSize: 12; color: successColor }
                }
            }
        }
    }
    
    // ========== 动画效果 ==========
    // 叫号闪烁动画
    SequentialAnimation {
        id: callAnimation
        running: true
        loops: Animation.Infinite
        
        PropertyAnimation {
            target: currentQueueNumber
            property: "color"
            from: accentColor
            to: "#FF8A65"
            duration: 500
        }
        
        PropertyAnimation {
            target: currentQueueNumber
            property: "color"
            from: "#FF8A65"
            to: accentColor
            duration: 500
        }
    }
    
    // ========== 定时刷新 ==========
    Timer {
        interval: 5000 // 5秒刷新一次
        running: true
        repeat: true
        onTriggered: {
            // 模拟更新排队数据
            refreshData()
        }
    }
    
    function refreshData() {
        // 实际应该从服务端获取数据
        // 这里简化处理
        var totalQueue = 0;
        for (var i = 0; i < deptQueueModel.count; i++) {
            var count = deptQueueModel.get(i).queueCount;
            // 随机变化
            count = Math.max(0, count + Math.floor(Math.random() * 3) - 1);
            deptQueueModel.setProperty(i, "queueCount", count);
            totalQueue += count;
        }
    }
    
    // ========== 键盘事件处理 ==========
    Keys.onEscapePressed: {
        Qt.quit()
    }
    
    // 全屏切换
    Keys.onPressed: {
        if (event.key === Qt.Key_F) {
            if (mainWindow.visibility === Window.FullScreen) {
                mainWindow.visibility = Window.Windowed
            } else {
                mainWindow.visibility = Window.FullScreen
            }
        }
    }
}
