import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "充电桩详情"
    property int chargerId: 0
    property var chargerData: ({})
    property bool isLoading: false
    
    Component.onCompleted: {
        if (chargerId > 0) {
            loadData()
        }
    }
    
    function loadData() {
        if (net.connected && chargerId > 0) {
            isLoading = true
            net.getChargerDetail(chargerId)
        }
    }
    
    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "2002" && resp.status === "ok") {
                chargerData = resp.data || {}
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            toast.showError("加载失败: " + msg)
        }
    }
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // ========== 顶部标题栏 ==========
        Rectangle {
            width: parent.width
            height: 56
            color: "#4A90D9"
            
            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                spacing: 8
                
                Text {
                    text: "←"
                    color: "white"
                    font.pixelSize: 24
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.stackView.pop()
                }
                
                Text {
                    text: "充电桩详情"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        
        Flickable {
            width: parent.width
            height: parent.height - 56
            contentHeight: contentColumn.height
            clip: true
            
            Column {
                id: contentColumn
                spacing: 0
                
                // ========== 充电桩信息卡片 ==========
                Rectangle {
                    width: parent.width
                    color: "#4A90D9"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Row {
                            width: parent.width
                            Text {
                                text: "⚡ " + (chargerData.name || ("充电桩 #" + chargerId))
                                color: "white"
                                font.bold: true
                                font.pixelSize: 20
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Rectangle {
                                width: 60
                                height: 26
                                radius: 13
                                color: chargerData.status === "online" ? "#4CAF50" : "#E53935"
                                
                                Text {
                                    text: chargerData.status === "online" ? "空闲" : "占用"
                                    color: "white"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.centerIn: parent
                                }
                            }
                        }
                        
                        Text {
                            text: "📍 " + (chargerData.location || "位置未知")
                            color: "#BBDEFB"
                            font.pixelSize: 14
                        }
                        
                        Text {
                            text: "🔖 序列号: " + (chargerData.serial_number || "未知")
                            color: "#BBDEFB"
                            font.pixelSize: 13
                        }
                    }
                }
                
                // ========== 充电参数 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16
                        
                        Text {
                            text: "充电参数"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }
                        
                        Row {
                            width: parent.width
                            spacing: 16
                            
                            Rectangle {
                                width: (parent.width - 16) / 2
                                height: 70
                                radius: 12
                                color: "#E3F2FD"
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        text: "⚡"
                                        font.pixelSize: 24
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: (chargerData.power_kw || chargerData.power || 60) + " kW"
                                        color: "#1976D2"
                                        font.bold: true
                                        font.pixelSize: 16
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "充电功率"
                                        color: "#666"
                                        font.pixelSize: 12
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                            
                            Rectangle {
                                width: (parent.width - 16) / 2
                                height: 70
                                radius: 12
                                color: "#FFF3E0"
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        text: "💰"
                                        font.pixelSize: 24
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "¥" + ((chargerData.price_kwh || chargerData.price || 1.2)).toFixed(2)
                                        color: "#F57C00"
                                        font.bold: true
                                        font.pixelSize: 16
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "元/度"
                                        color: "#666"
                                        font.pixelSize: 12
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 费用说明 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "费用说明"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }
                        
                        Rectangle {
                            width: parent.width
                            height: 100
                            radius: 12
                            color: "white"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "电费"
                                        color: "#666"
                                        font.pixelSize: 13
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + ((chargerData.price_kwh || 1.2)).toFixed(2) + "/度"
                                        color: "#333"
                                        font.size: 13
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "服务费"
                                        color: "#666"
                                        font.pixelSize: 13
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥0.10/度"
                                        color: "#333"
                                        font.size: 13
                                    }
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#F0F0F0"
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "合计"
                                        color: "#333"
                                        font.bold: true
                                        font.size: 13
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + (((chargerData.price_kwh || 1.2) + 0.1)).toFixed(2) + "/度"
                                        color: "#FF9800"
                                        font.bold: true
                                        font.size: 14
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 峰谷电价提示 ==========
                Rectangle {
                    width: parent.width
                    color: "#FFF8E1"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        
                        Row {
                            spacing: 8
                            Text {
                                text: "⏰"
                                font.pixelSize: 18
                            }
                            Text {
                                text: "峰谷电价"
                                font.bold: true
                                font.pixelSize: 14
                                color: "#F57C00"
                            }
                        }
                        
                        Text {
                            text: "• 谷时 (0:00-6:00): ¥0.30/度"
                            color: "#666"
                            font.pixelSize: 12
                        }
                        Text {
                            text: "• 平段 (6:00-9:00, 12:00-14:00, 18:00-21:00): ¥0.60/度"
                            color: "#666"
                            font.pixelSize: 12
                        }
                        Text {
                            text: "• 峰时 (9:00-12:00, 14:00-18:00): ¥1.20/度"
                            color: "#666"
                            font.pixelSize: 12
                        }
                    }
                }
                
                // ========== 开始充电按钮 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    anchors.topMargin: 16
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Rectangle {
                            width: parent.width
                            height: 50
                            radius: 25
                            color: chargerData.status === "online" ? "#4CAF50" : "#CCCCCC"
                            
                            Text {
                                text: chargerData.status === "online" ? "⚡ 开始充电" : "充电桩不可用"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                enabled: chargerData.status === "online"
                                onClicked: {
                                    net.startCharge(chargerId, "auto", 100)
                                    toast.showSuccess("正在启动充电...")
                                }
                            }
                        }
                        
                        Text {
                            text: "请确保充电枪已正确连接"
                            color: "#999"
                            font.pixelSize: 12
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                
                // 底部间距
                Item { height: 100 }
            }
        }
    }
    
    header: null
}
