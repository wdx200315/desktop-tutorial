import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

Rectangle {
    id: bottomNav
    width: parent ? parent.width : 360
    height: 65
    color: "#FFFFFF"
    anchors.bottom: parent ? parent.bottom : undefined
    
    // 顶部阴影
    Rectangle {
        width: parent ? parent.width : 360
        height: 1
        color: "#E0E0E0"
        anchors.top: parent.top
    }
    
    // 导航项数据
    property list<var> navItems: [
        { icon: "🏠", activeIcon: "🏠", label: "首页" },
        { icon: "🔍", activeIcon: "🔍", label: "发现" },
        { icon: "📷", activeIcon: "📷", label: "充电" },
        { icon: "💬", activeIcon: "💬", label: "消息" },
        { icon: "👤", activeIcon: "👤", label: "我的" }
    ]
    
    // 当前选中索引
    property int currentIndex: 0
    signal tabClicked(int index)
    
    // 消息数量（用于显示红点）
    property int messageCount: 0
    
    // 充电状态（是否正在充电）
    property bool isCharging: false
    
    // 动画属性
    property real animProgress: 0.0
    
    Row {
        anchors.fill: parent
        anchors.topMargin: 4
        
        Repeater {
            model: navItems.length
            
            Rectangle {
                width: bottomNav.width / navItems.length
                height: parent.height
                color: "transparent"
                
                // 选中指示器
                Rectangle {
                    id: indicator
                    width: 40
                    height: 3
                    radius: 1.5
                    color: "#4A90D9"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    visible: index === currentIndex
                    
                    // 指示器动画
                    PropertyAnimation on opacity {
                        from: 0
                        to: 1
                        duration: 200
                    }
                }
                
                // 中心内容
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    // 图标和红点
                    Item {
                        width: 32
                        height: 32
                        anchors.horizontalCenter: parent.horizontalCenter
                        
                        // 背景圆圈（选中时显示）
                        Rectangle {
                            id: iconBg
                            width: 36
                            height: 36
                            radius: 18
                            color: index === currentIndex ? "#E3F2FD" : "transparent"
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: index === 2 ? -4 : 0
                            
                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                        
                        // 图标
                        Text {
                            id: navIcon
                            text: index === currentIndex ? navItems[index].activeIcon : navItems[index].icon
                            font.pixelSize: index === 2 ? 28 : 24
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: index === 2 ? -4 : 0
                            
                            // 图标动画
                            PropertyAnimation on scale {
                                from: 0.8
                                to: 1.0
                                duration: 200
                            }
                        }
                        
                        // 充电状态指示器
                        Rectangle {
                            visible: index === 2 && isCharging
                            width: 8
                            height: 8
                            radius: 4
                            color: "#4CAF50"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: index === 2 ? 2 : 4
                            anchors.rightMargin: index === 2 ? 6 : 8
                            
                            // 呼吸动画
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                PropertyAnimation { from: 1; to: 0.3; duration: 800 }
                                PropertyAnimation { from: 0.3; to: 1; duration: 800 }
                            }
                        }
                        
                        // 消息红点
                        Rectangle {
                            visible: index === 3 && messageCount > 0
                            width: messageCount > 99 ? 22 : 18
                            height: 18
                            radius: 9
                            color: "#E53935"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: index === 2 ? 0 : 2
                            anchors.rightMargin: index === 2 ? 4 : 6
                            
                            // 红点动画
                            PropertyAnimation on scale {
                                from: 0
                                to: 1
                                duration: 300
                                easing.type: Easing.OutBack
                            }
                            
                            Text {
                                text: messageCount > 99 ? "99+" : messageCount.toString()
                                color: "white"
                                font.pixelSize: 9
                                font.bold: true
                                font.family: "Arial"
                                anchors.centerIn: parent
                            }
                        }
                        
                        // 扫码按钮特殊样式
                        Rectangle {
                            visible: index === 2
                            width: 52
                            height: 52
                            radius: 26
                            color: isCharging ? "#4CAF50" : "#4A90D9"
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: -6
                            
                            // 外圈动画（充电中）
                            Rectangle {
                                visible: isCharging
                                anchors.fill: parent
                                radius: parent.radius + 4
                                color: "transparent"
                                border.width: 2
                                border.color: "#4CAF50"
                                opacity: 0.5
                                
                                SequentialAnimation on scale {
                                    loops: Animation.Infinite
                                    PropertyAnimation { from: 1; to: 1.3; duration: 1000 }
                                    PropertyAnimation { from: 1.3; to: 1; duration: 1000 }
                                }
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    PropertyAnimation { from: 0.5; to: 0; duration: 1000 }
                                    PropertyAnimation { from: 0; to: 0.5; duration: 1000 }
                                }
                            }
                            
                            // 按钮内阴影
                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius
                                color: "transparent"
                                border.width: 3
                                border.color: "#FFFFFF33"
                            }
                            
                            Text {
                                text: isCharging ? "⚡" : "📷"
                                font.pixelSize: 24
                                color: "white"
                                anchors.centerIn: parent
                            }
                        }
                    }
                    
                    // 标签文字
                    Text {
                        id: navLabel
                        text: navItems[index].label
                        font.pixelSize: index === 2 ? 9 : 11
                        font.family: "Microsoft YaHei"
                        color: index === currentIndex ? "#4A90D9" : "#666666"
                        font.bold: index === currentIndex
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: index === 2 ? 6 : 2
                        
                        // 文字颜色动画
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                    }
                }
                
                // 点击区域
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (index !== currentIndex) {
                            currentIndex = index
                            tabClicked(index)
                        }
                    }
                }
            }
        }
    }
    
    // 点击反馈
    Rectangle {
        id: clickFeedback
        width: 50
        height: 50
        radius: 25
        color: "#4A90D9"
        opacity: 0
        anchors.centerIn: parent
        visible: false
        
        PropertyAnimation {
            target: clickFeedback
            property: "opacity"
            from: 0.3
            to: 0
            duration: 300
        }
    }
}
