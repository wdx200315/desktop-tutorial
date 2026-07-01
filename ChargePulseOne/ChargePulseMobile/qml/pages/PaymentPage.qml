import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "支付订单"
    property int orderId: 0
    property double amount: 0.0
    property double couponDiscount: 0.0
    property int selectedPayMethod: 0  // 0=余额, 1=微信, 2=支付宝, 3=银行卡
    property bool isPaying: false
    property bool hasCoupon: false
    property bool showSuccess: false
    property var orderInfo: ({})
    
    // 模拟订单数据
    property var mockOrder: ({
        charger_name: "充电桩 #1",
        charger_location: "北京市朝阳区某停车场",
        start_time: "2024-01-15 14:30",
        end_time: "2024-01-15 16:45",
        duration: "2小时15分",
        energy: 45.6,
        energy_fee: 32.50,
        service_fee: 4.56,
        total_fee: 37.06
    })

    Component.onCompleted: {
        // 如果没有传入orderId，使用模拟数据
        if (orderId === 0) {
            amount = mockOrder.total_fee
        }
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "4004" && resp.status === "ok") {
                isPaying = false
                if (resp.data.status === "paid") {
                    showSuccess = true
                    toast.showSuccess("支付成功！")
                } else {
                    toast.showError("支付失败，请重试")
                }
            }
        }
        function onErrorOccurred(msg) {
            isPaying = false
            toast.showError("支付失败: " + msg)
        }
    }
    
    function getActualAmount() {
        return Math.max(0, amount - couponDiscount).toFixed(2)
    }
    
    function getUserBalance() {
        return 100.00  // 模拟余额
    }
    
    function canUseBalance() {
        return getUserBalance() >= getActualAmount()
    }
    
    function performPayment() {
        if (isPaying) return
        
        isPaying = true
        
        // 根据支付方式执行支付
        if (selectedPayMethod === 0) {
            // 余额支付
            if (!canUseBalance()) {
                toast.showWarning("余额不足，请选择其他支付方式")
                isPaying = false
                return
            }
        }
        
        // 发送支付请求
        var req = {
            "cmd": "4004",
            "data": { 
                "action": "pay", 
                "order_id": orderId, 
                "amount": getActualAmount(),
                "pay_method": selectedPayMethod,
                "coupon_id": hasCoupon ? "CUSTOM001" : ""
            },
            "token": session.token
        }
        net.sendRequest(req)
        
        // 模拟支付成功（用于测试）
        Qt.callLater(function() {
            if (isPaying) {
                showSuccess = true
                isPaying = false
            }
        }, 2000)
    }

    Column {
        anchors.fill: parent
        spacing: 0
        
        // ========== 顶部标题栏 ==========
        Rectangle {
            width: parent.width
            height: 56
            color: showSuccess ? "#4CAF50" : "#4A90D9"
            
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
                    onClicked: {
                        if (showSuccess) {
                            root.stackView.clear()
                            root.stackView.push(mainPageComponent)
                        } else {
                            root.stackView.pop()
                        }
                    }
                }
                
                Text {
                    text: showSuccess ? "支付成功" : "订单支付"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        
        Flickable {
            visible: !showSuccess
            width: parent.width
            height: parent.height - 56
            contentHeight: contentColumn.height
            clip: true
            
            Column {
                id: contentColumn
                spacing: 0
                
                // ========== 订单摘要 ==========
                Rectangle {
                    width: parent.width
                    color: "#4A90D9"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12
                        
                        Text {
                            text: "订单金额"
                            color: "#BBDEFB"
                            font.pixelSize: 14
                        }
                        
                        Row {
                            spacing: 4
                            
                            Text {
                                text: "¥"
                                color: "white"
                                font.pixelSize: 24
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Text {
                                text: amount.toFixed(2)
                                color: "white"
                                font.bold: true
                                font.pixelSize: 42
                            }
                        }
                        
                        Text {
                            text: "订单号: " + (orderId > 0 ? orderId : "CP" + Date.now())
                            color: "#BBDEFB"
                            font.pixelSize: 12
                        }
                    }
                }
                
                // ========== 订单详情 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "📋 订单详情"
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
                                        text: "充电桩"
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: mockOrder.charger_name
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "位置"
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: mockOrder.charger_location
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "充电时长"
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: mockOrder.duration
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "已充电量"
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: mockOrder.energy.toFixed(2) + " kWh"
                                        font.pixelSize: 13
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
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + mockOrder.energy_fee.toFixed(2)
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                }
                                
                                Row {
                                    width: parent.width
                                    Text {
                                        text: "服务费"
                                        font.pixelSize: 13
                                        color: "#666"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: "¥" + mockOrder.service_fee.toFixed(2)
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 优惠券 ==========
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
                                text: "🎫 优惠券"
                                font.bold: true
                                font.pixelSize: 14
                                color: "#333"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: hasCoupon ? "- ¥" + couponDiscount.toFixed(2) : "暂无可用"
                                font.pixelSize: 13
                                color: hasCoupon ? "#4CAF50" : "#999"
                            }
                        }
                        
                        Rectangle {
                            width: parent.width
                            height: hasCoupon ? 50 : 44
                            radius: 8
                            color: hasCoupon ? "#E8F5E9" : "#F5F7FA"
                            border.width: 1
                            border.color: hasCoupon ? "#4CAF50" : "#E0E0E0"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                
                                Text {
                                    text: hasCoupon ? "🎫" : "📭"
                                    font.pixelSize: 20
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: hasCoupon ? "新人专享券" : "暂无优惠券可用"
                                        font.pixelSize: 13
                                        color: "#333"
                                    }
                                    
                                    Text {
                                        text: hasCoupon ? "满30元减5元" : "更多优惠敬请期待"
                                        font.pixelSize: 11
                                        color: "#666"
                                        visible: hasCoupon
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                // 切换优惠券
                                Rectangle {
                                    width: 48
                                    height: 24
                                    radius: 12
                                    color: hasCoupon ? "#4CAF50" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Rectangle {
                                        width: 20
                                        height: 20
                                        radius: 10
                                        color: "white"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: hasCoupon ? 26 : 2
                                        Behavior on anchors.leftMargin { PropertyAnimation { duration: 200 } }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            hasCoupon = !hasCoupon
                                            couponDiscount = hasCoupon ? 5.00 : 0.00
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // ========== 支付方式 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        Text {
                            text: "💳 选择支付方式"
                            font.bold: true
                            font.pixelSize: 14
                            color: "#333"
                        }
                        
                        // 余额支付
                        Rectangle {
                            width: parent.width
                            height: 60
                            radius: 12
                            color: selectedPayMethod === 0 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedPayMethod === 0 ? 2 : 1
                            border.color: selectedPayMethod === 0 ? "#4A90D9" : "#E0E0E0"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                
                                Rectangle {
                                    width: 40
                                    height: 40
                                    radius: 8
                                    color: "#4CAF50"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "💰"
                                        font.pixelSize: 20
                                        anchors.centerIn: parent
                                    }
                                }
                                
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "账户余额"
                                        font.pixelSize: 14
                                        color: "#333"
                                    }
                                    
                                    Text {
                                        text: "可用余额: ¥" + getUserBalance().toFixed(2)
                                        font.pixelSize: 12
                                        color: canUseBalance() ? "#4CAF50" : "#E53935"
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                // 余额不足提示
                                Text {
                                    text: selectedPayMethod === 0 && !canUseBalance() ? "余额不足" : ""
                                    font.pixelSize: 11
                                    color: "#E53935"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                
                                Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: selectedPayMethod === 0 ? "#4A90D9" : "transparent"
                                    border.width: 2
                                    border.color: selectedPayMethod === 0 ? "#4A90D9" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "✓"
                                        font.pixelSize: 12
                                        color: "white"
                                        anchors.centerIn: parent
                                        visible: selectedPayMethod === 0
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedPayMethod = 0
                            }
                        }
                        
                        // 微信支付
                        Rectangle {
                            width: parent.width
                            height: 60
                            radius: 12
                            color: selectedPayMethod === 1 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedPayMethod === 1 ? 2 : 1
                            border.color: selectedPayMethod === 1 ? "#4A90D9" : "#E0E0E0"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                
                                Rectangle {
                                    width: 40
                                    height: 40
                                    radius: 8
                                    color: "#07C160"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "微"
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: "white"
                                        anchors.centerIn: parent
                                    }
                                }
                                
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "微信支付"
                                        font.pixelSize: 14
                                        color: "#333"
                                    }
                                    
                                    Text {
                                        text: "推荐"
                                        font.pixelSize: 10
                                        color: "white"
                                        visible: selectedPayMethod === 1
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: selectedPayMethod === 1 ? "#4A90D9" : "transparent"
                                    border.width: 2
                                    border.color: selectedPayMethod === 1 ? "#4A90D9" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "✓"
                                        font.pixelSize: 12
                                        color: "white"
                                        anchors.centerIn: parent
                                        visible: selectedPayMethod === 1
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedPayMethod = 1
                            }
                        }
                        
                        // 支付宝
                        Rectangle {
                            width: parent.width
                            height: 60
                            radius: 12
                            color: selectedPayMethod === 2 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedPayMethod === 2 ? 2 : 1
                            border.color: selectedPayMethod === 2 ? "#4A90D9" : "#E0E0E0"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                
                                Rectangle {
                                    width: 40
                                    height: 40
                                    radius: 8
                                    color: "#1677FF"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "支"
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: "white"
                                        anchors.centerIn: parent
                                    }
                                }
                                
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "支付宝"
                                        font.pixelSize: 14
                                        color: "#333"
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: selectedPayMethod === 2 ? "#4A90D9" : "transparent"
                                    border.width: 2
                                    border.color: selectedPayMethod === 2 ? "#4A90D9" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "✓"
                                        font.pixelSize: 12
                                        color: "white"
                                        anchors.centerIn: parent
                                        visible: selectedPayMethod === 2
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedPayMethod = 2
                            }
                        }
                    }
                }
                
                // ========== 底部支付按钮 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12
                        
                        // 应付金额
                        Row {
                            width: parent.width
                            spacing: 8
                            
                            Text {
                                text: "实付金额:"
                                font.pixelSize: 14
                                color: "#666"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "¥" + getActualAmount()
                                font.bold: true
                                font.pixelSize: 24
                                color: "#FF9800"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        
                        // 支付按钮
                        Rectangle {
                            width: parent.width
                            height: 54
                            radius: 27
                            color: isPaying ? "#CCCCCC" : "#4CAF50"
                            
                            Row {
                                anchors.centerIn: parent
                                spacing: 8
                                
                                Text {
                                    text: isPaying ? "支付中..." : "确认支付 ¥" + getActualAmount()
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 17
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                
                                // 加载动画
                                Rectangle {
                                    visible: isPaying
                                    width: 20
                                    height: 20
                                    radius: 10
                                    color: "transparent"
                                    border.width: 2
                                    border.color: "#FFFFFF"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    RotationAnimation on rotation {
                                        loops: Animation.Infinite
                                        from: 0
                                        to: 360
                                        duration: 800
                                        running: isPaying
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                enabled: !isPaying
                                onClicked: performPayment()
                            }
                        }
                        
                        Text {
                            text: "点击支付即表示同意《支付服务协议》"
                            font.pixelSize: 10
                            color: "#999"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                
                Item { height: 100 }
            }
        }
        
        // ========== 支付成功页面 ==========
        Rectangle {
            visible: showSuccess
            anchors.fill: parent
            color: "#4CAF50"
            
            Column {
                anchors.centerIn: parent
                spacing: 24
                
                // 成功动画
                Rectangle {
                    width: 100
                    height: 100
                    radius: 50
                    color: "#FFFFFF22"
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        width: 80
                        height: 80
                        radius: 40
                        color: "white"
                        anchors.centerIn: parent
                        
                        Text {
                            text: "✓"
                            font.pixelSize: 48
                            color: "#4CAF50"
                            anchors.centerIn: parent
                        }
                    }
                    
                    // 脉冲动画
                    Rectangle {
                        anchors.centerIn: parent
                        width: 120
                        height: 120
                        radius: 60
                        color: "transparent"
                        border.width: 3
                        border.color: "#FFFFFF"
                        opacity: 0.3
                        
                        SequentialAnimation on scale {
                            loops: Animation.Infinite
                            PropertyAnimation { from: 1; to: 1.3; duration: 1500 }
                            PropertyAnimation { from: 1.3; to: 1; duration: 1500 }
                        }
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PropertyAnimation { from: 0.3; to: 0; duration: 1500 }
                            PropertyAnimation { from: 0; to: 0.3; duration: 1500 }
                        }
                    }
                }
                
                Text {
                    text: "支付成功"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 28
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "¥" + getActualAmount()
                    color: "#FFD700"
                    font.bold: true
                    font.pixelSize: 36
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "感谢您的使用，欢迎下次光临"
                    color: "#BBDEFB"
                    font.pixelSize: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                // 返回首页按钮
                Rectangle {
                    width: 200
                    height: 48
                    radius: 24
                    color: "white"
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Text {
                        text: "返回首页"
                        color: "#4CAF50"
                        font.bold: true
                        font.pixelSize: 16
                        anchors.centerIn: parent
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.stackView.clear()
                            root.stackView.push(mainPageComponent)
                        }
                    }
                }
            }
        }
    }
    
    header: null
}
