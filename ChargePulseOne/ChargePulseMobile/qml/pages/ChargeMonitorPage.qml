import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "充电监控"
    property int orderId: 0
    property int chargerId: 0
    property var chargeData: ({
        status: "charging",
        energy: 0,
        power: 60,
        voltage: 380,
        current: 0,
        temperature: 30,
        soc: 20,
        progress: 0,
        fee: 0,
        duration: 0,
        price_period: "平段",
        current_price: 0.6,
        estimated_total: 50.0,
        estimated_remaining: 30.0
    })
    property bool isLoading: false
    property bool showStopConfirm: false
    
    // 定时刷新
    Timer {
        id: refreshTimer
        interval: 2000
        repeat: true
        onTriggered: {
            if (net.connected && orderId > 0) {
                net.getChargeStatus(orderId)
            }
        }
    }
    
    Component.onCompleted: {
        if (orderId > 0) {
            net.getChargeStatus(orderId)
            refreshTimer.start()
        }
    }
    
    Component.onDestruction: {
        refreshTimer.stop()
    }
    
    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "3003" && resp.status === "ok") {
                chargeData = resp.data || {}
            }
            if (resp.cmd === "3001" && resp.status === "ok") {
                orderId = resp.data?.order_id || orderId
                chargerId = resp.data?.charger_id || chargerId
            }
            // 停止充电成功
            if (resp.cmd === "3002" && resp.status === "ok") {
                refreshTimer.stop()
                toast.showSuccess("充电已停止")
                // 跳转支付页面
                root.stackView.push(paymentPageComponent, { "orderId": orderId })
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            toast.showError("加载失败: " + msg)
        }
    }
    
    function formatDuration(seconds) {
        var hours = Math.floor(seconds / 3600)
        var minutes = Math.floor((seconds % 3600) / 60)
        var secs = seconds % 60
        if (hours > 0) {
            return hours + "小时" + minutes + "分"
        } else if (minutes > 0) {
            return minutes + "分" + secs + "秒"
        }
        return secs + "秒"
    }
    
    function formatTime(seconds) {
        var hours = Math.floor(seconds / 3600)
        var minutes = Math.floor((seconds % 3600) / 60)
        if (hours > 0) {
            return hours + ":" + (minutes < 10 ? "0" : "") + minutes
        }
        return "00:" + (minutes < 10 ? "0" : "") + minutes
    }
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // ========== 顶部标题栏 ==========
        Rectangle {
            width: parent.width
            height: 56
            color: chargeData.status === "charging" ? "#4CAF50" : "#4A90D9"
            
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
                    text: chargeData.status === "charging" ? "充电中" : "充电完成"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Item { Layout.fillWidth: true }
                
                // 刷新指示器
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#4CAF50"
                    anchors.verticalCenter: parent.verticalCenter
                    
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1; to: 0.3; duration: 500 }
                        NumberAnimation { from: 0.3; to: 1; duration: 500 }
                    }
                }
                
                Text {
                    text: " 实时监控"
                    color: "#BBDEFB"
                    font.pixelSize: 12
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
                
                // ========== 充电状态核心区域 ==========
                Rectangle {
                    width: parent.width
                    height: 220
                    color: chargeData.status === "charging" ? "#4CAF50" : "#4A90D9"
                    
                    // 背景装饰
                    Rectangle {
                        width: 200
                        height: 200
                        radius: 100
                        color: "#FFFFFF"
                        opacity: 0.05
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: -80
                    }
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 8
                        
                        // 充电动画圆环
                        Item {
                            width: parent.width
                            height: 100
                            anchors.horizontalCenter: parent.horizontalCenter
                            
                            // 外圈
                            Rectangle {
                                width: 100
                                height: 100
                                radius: 50
                                color: "transparent"
                                border.width: 6
                                border.color: "#FFFFFF33"
                                anchors.centerIn: parent
                            }
                            
                            // 进度圈
                            Rectangle {
                                width: 100
                                height: 100
                                radius: 50
                                color: "transparent"
                                border.width: 6
                                border.color: "white"
                                anchors.centerIn: parent
                                
                                // 截断效果模拟进度
                                Rectangle {
                                    width: 100
                                    height: 100
                                    radius: 50
                                    color: "#4CAF50"
                                    anchors.centerIn: parent
                                    rotation: -90
                                    transformOrigin: Item.Center
                                    
                                    // 只显示进度的部分
                                    Rectangle {
                                        anchors.right: parent.right
                                        width: 50
                                        height: 100
                                        color: "transparent"
                                        clip: true
                                        
                                        Rectangle {
                                            width: 50
                                            height: 100
                                            radius: 25
                                            color: chargeData.status === "charging" ? "#4CAF50" : "#4A90D9"
                                            anchors.right: parent.right
                                            rotation: (chargeData.progress / 100) * 360
                                            transformOrigin: Item.Left
                                        }
                                    }
                                }
                                
                                // 中心文字
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 2
                                    
                                    Text {
                                        text: chargeData.progress.toFixed(0) + "%"
                                        color: "white"
                                        font.bold: true
                                        font.pixelSize: 28
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: chargeData.status === "charging" ? "充电中" : "已完成"
                                        color: "#BBDEFB"
                                        font.pixelSize: 12
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                            
                            // 脉冲动画（充电中）
                            Rectangle {
                                visible: chargeData.status === "charging"
                                width: 120
                                height: 120
                                radius: 60
                                color: "transparent"
                                border.width: 2
                                border.color: "#FFFFFF"
                                opacity: 0.3
                                anchors.centerIn: parent
                                
                                SequentialAnimation on scale {
                                    loops: Animation.Infinite
                                    PropertyAnimation { from: 1; to: 1.3; duration: 1500; easing.type: Easing.OutQuad }
                                    PropertyAnimation { from: 1.3; to: 1; duration: 1500; easing.type: Easing.InQuad }
                                }
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    PropertyAnimation { from: 0.3; to: 0; duration: 1500 }
                                    PropertyAnimation { from: 0; to: 0.3; duration: 1500 }
                                }
                            }
                        }
                        
                        // 充电数据
                        Row {
                            width: parent.width
                            spacing: 20
                            
                            Column {
                                spacing: 2
                                anchors.horizontalCenter: parent.horizontalCenter
                                
                                Text {
                                    text: "⚡ " + chargeData.energy.toFixed(2) + " kWh"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Text {
                                    text: "已充电量"
                                    color: "#BBDEFB"
                                    font.pixelSize: 11
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                            
                            Rectangle {
                                width: 1
                                height: 30
                                color: "#FFFFFF33"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Column {
                                spacing: 2
                                anchors.horizontalCenter: parent.horizontalCenter
                                
                                Text {
                                    text: formatTime(chargeData.duration || 0)
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Text {
                                    text: "充电时长"
                                    color: "#BBDEFB"
                                    font.pixelSize: 11
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                            
                            Rectangle {
                                width: 1
                                height: 30
                                color: "#FFFFFF33"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Column {
                                spacing: 2
                                anchors.horizontalCenter: parent.horizontalCenter
                                
                                Text {
                                    text: "¥" + (chargeData.fee || 0).toFixed(2)
                                    color: "#FFD700"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Text {
                                    text: "当前费用"
                                    color: "#BBDEFB"
                                    font.pixelSize: 11
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
                
                // ========== 温度警告提示 ==========
                Rectangle {
                    width: parent.width
                    height: chargeData.temperature > 45 ? 50 : 0
                    color: "#FFEBEE"
                    clip: true
                    Behavior on height { NumberAnimation { duration: 300 } }
                    visible: chargeData.temperature > 45
                    
                    Row {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8
                        
                        Text {
                            text: "⚠️"
                            font.pixelSize: 20
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        Column {
                            spacing: 2
                            anchors.verticalCenter: parent.verticalCenter
                            
                            Text {
                                text: "温度偏高，请注意安全"
                                color: "#E53935"
                                font.bold: true
                                font.pixelSize: 13
                            }
                            
                            Text {
                                text: "当前温度: " + chargeData.temperature.toFixed(1) + "°C，建议停止充电"
                                color: "#666"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
                
                // ========== 实时参数 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Row {
                            width: parent.width
                            
                            Text {
                                text: "📊 实时参数"
                                font.bold: true
                                font.pixelSize: 14
                                color: "#333"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "更新于 " + new Date().toLocaleTimeString()
                                color: "#999"
                                font.pixelSize: 10
                            }
                        }
                        
                        Grid {
                            columns: 2
                            spacing: 12
                            width: parent.width
                            
                            // 功率
                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 80
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
                                        text: chargeData.power.toFixed(1) + " kW"
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: "#4A90D9"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "充电功率"
                                        font.pixelSize: 11
                                        color: "#666"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                            
                            // 电压
                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 80
                                radius: 12
                                color: "#E8F5E9"
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        text: "🔌"
                                        font.pixelSize: 24
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: chargeData.voltage.toFixed(0) + " V"
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: "#4CAF50"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "当前电压"
                                        font.pixelSize: 11
                                        color: "#666"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                            
                            // 电流
                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 80
                                radius: 12
                                color: "#FFF3E0"
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        text: "⚡"
                                        font.pixelSize: 24
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: chargeData.current.toFixed(1) + " A"
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: "#FF9800"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "充电电流"
                                        font.pixelSize: 11
                                        color: "#666"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                            
                            // 温度
                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 80
                                radius: 12
                                color: chargeData.temperature > 45 ? "#FFEBEE" : "#E3F2FD"
                                
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    
                                    Text {
                                        text: chargeData.temperature > 45 ? "⚠️" : "🌡️"
                                        font.pixelSize: 24
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: chargeData.temperature.toFixed(1) + " °C"
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: chargeData.temperature > 45 ? "#E53935" : "#4A90D9"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "桩体温度"
                                        font.pixelSize: 11
                                        color: "#666"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 时段电价 ==========
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
                                text: "当前时段: " + (chargeData.price_period || "平段")
                                font.bold: true
                                font.pixelSize: 14
                                color: "#F57C00"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            // 时段标签
                            Rectangle {
                                height: 22
                                radius: 11
                                color: "#FFE0B2"
                                
                                Text {
                                    text: "电价浮动中"
                                    font.pixelSize: 10
                                    color: "#E65100"
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 8
                                }
                            }
                        }
                        
                        Row {
                            width: parent.width
                            
                            Text {
                                text: "基础电价: ¥" + (chargeData.current_price || 0.6).toFixed(2) + "/度"
                                font.size: 12
                                color: "#666"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "服务费: ¥0.10/度"
                                font.size: 12
                                color: "#666"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "合计: ¥" + ((chargeData.current_price || 0.6) + 0.1).toFixed(2) + "/度"
                                font.size: 12
                                font.bold: true
                                color: "#F57C00"
                            }
                        }
                    }
                }
                
                // ========== 费用明细 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "💰 费用明细"
                            font.bold: true
                            font.pixelSize: 14
                            color: "#333"
                        }
                        
                        Rectangle {
                            width: parent.width
                            radius: 12
                            color: "#F5F7FA"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "充电时长"
                                        font.size: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: formatDuration(chargeData.duration || 0)
                                        font.size: 13
                                        font.bold: true
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "已充电量"
                                        font.size: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: chargeData.energy.toFixed(2) + " kWh"
                                        font.size: 13
                                        font.bold: true
                                        color: "#333"
                                    }
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#E0E0E0"
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "电费"
                                        font.size: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + (chargeData.energy_fee || 0).toFixed(2)
                                        font.size: 13
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "服务费"
                                        font.size: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + (chargeData.service_fee || 0).toFixed(2)
                                        font.size: 13
                                        color: "#333"
                                    }
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 1
                                    color: "#E0E0E0"
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "当前费用"
                                        font.bold: true
                                        font.size: 15
                                        color: "#333"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + (chargeData.fee || 0).toFixed(2)
                                        font.bold: true
                                        font.size: 20
                                        color: "#FF9800"
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 停止充电按钮 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        // 主按钮
                        Rectangle {
                            width: parent.width
                            height: 54
                            radius: 27
                            color: "#E53935"
                            
                            Row {
                                anchors.centerIn: parent
                                spacing: 8
                                
                                Text {
                                    text: "⏹ 停止充电"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 17
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    showStopConfirm = true
                                }
                            }
                        }
                        
                        Text {
                            text: "停止充电后将自动结算当前费用"
                            font.size: 12
                            color: "#999"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                
                // 底部间距
                Item { height: 100 }
            }
        }
        
        // ========== 停止充电确认对话框 ==========
        Rectangle {
            visible: showStopConfirm
            anchors.fill: parent
            color: "#80000000"
            
            Rectangle {
                width: parent.width - 48
                height: 280
                radius: 20
                color: "white"
                anchors.centerIn: parent
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16
                    
                    // 警告图标
                    Rectangle {
                        width: 60
                        height: 60
                        radius: 30
                        color: "#FFEBEE"
                        anchors.horizontalCenter: parent.horizontalCenter
                        
                        Text {
                            text: "⚠️"
                            font.pixelSize: 32
                            anchors.centerIn: parent
                        }
                    }
                    
                    Text {
                        text: "确认停止充电？"
                        font.bold: true
                        font.pixelSize: 18
                        color: "#333"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    // 充电摘要
                    Rectangle {
                        width: parent.width
                        radius: 12
                        color: "#F5F7FA"
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8
                            
                            Row {
                                width: parent.width
                                Text {
                                    text: "充电时长"
                                    font.size: 13
                                    color: "#666"
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: formatDuration(chargeData.duration || 0)
                                    font.size: 13
                                    color: "#333"
                                }
                            }
                            
                            Row {
                                width: parent.width
                                Text {
                                    text: "已充电量"
                                    font.size: 13
                                    color: "#666"
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: chargeData.energy.toFixed(2) + " kWh"
                                    font.size: 13
                                    color: "#333"
                                }
                            }
                            
                            Row {
                                width: parent.width
                                Text {
                                    text: "应付金额"
                                    font.bold: true
                                    font.size: 14
                                    color: "#333"
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: "¥" + (chargeData.fee || 0).toFixed(2)
                                    font.bold: true
                                    font.size: 16
                                    color: "#FF9800"
                                }
                            }
                        }
                    }
                    
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#F0F0F0"
                    }
                    
                    Row {
                        spacing: 12
                        width: parent.width
                        
                        Rectangle {
                            width: (parent.width - 12) / 2
                            height: 48
                            radius: 24
                            color: "#F5F7FA"
                            
                            Text {
                                text: "取消"
                                color: "#666"
                                font.bold: true
                                font.pixelSize: 15
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: showStopConfirm = false
                            }
                        }
                        
                        Rectangle {
                            width: (parent.width - 12) / 2
                            height: 48
                            radius: 24
                            color: "#E53935"
                            
                            Text {
                                text: "确认停止"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 15
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    showStopConfirm = false
                                    if (orderId > 0) {
                                        net.stopCharge(orderId)
                                        toast.showInfo("正在停止充电...")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    header: null
}
