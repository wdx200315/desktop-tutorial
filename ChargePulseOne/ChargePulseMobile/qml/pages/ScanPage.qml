import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtMultimedia 6.0
import "../components"

Page {
    title: "扫码充电"
    property string scanResult: ""
    property bool isScanning: true
    property bool hasResult: false
    property int scannedChargerId: 0
    property var chargerData: ({})
    property int selectedMode: 0  // 0=自动充满, 1=定时, 2=按电量
    property int chargeDuration: 60  // 分钟
    property double chargeAmount: 30  // 度
    property bool isStarting: false

    Component.onCompleted: {
        isScanning = true
        hasResult = false
        selectedMode = 0
    }

    function handleScanResult(result) {
        if (hasResult) return
        scanResult = result
        isScanning = false
        hasResult = true
        parseQRCode(result)
    }

    function parseQRCode(code) {
        // 二维码格式: "ChargePulse:charger_id:serial_number"
        // 例如: "ChargePulse:1:CP-001"
        var parts = code.split(":")
        if (parts.length >= 2) {
            scannedChargerId = parseInt(parts[1])
            if (net.connected && scannedChargerId > 0) {
                net.getChargerDetail(scannedChargerId)
                toast.show("正在验证充电桩...")
            }
        } else {
            toast.showWarning("无效的充电桩二维码")
            isScanning = true
            hasResult = false
        }
    }

    function getEstimatedCost() {
        var price = chargerData.price_kwh || chargerData.price || 1.2
        var power = chargerData.power_kw || chargerData.power || 60
        
        if (selectedMode === 0) {
            // 自动充满 - 假设充到80%需要的时间
            var estimatedKwh = power * 1.0  // 粗略估计1小时充满
            return (estimatedKwh * price).toFixed(2)
        } else if (selectedMode === 1) {
            // 定时模式
            var hours = chargeDuration / 60
            var estimatedKwh = power * hours
            return (estimatedKwh * price).toFixed(2)
        } else {
            // 按电量模式
            return (chargeAmount * price).toFixed(2)
        }
    }

    function getEstimatedKwh() {
        var power = chargerData.power_kw || chargerData.power || 60
        
        if (selectedMode === 0) {
            return power * 1.0  // 粗略估计1小时
        } else if (selectedMode === 1) {
            var hours = chargeDuration / 60
            return power * hours
        } else {
            return chargeAmount
        }
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "2002" && resp.status === "ok") {
                chargerData = resp.data || {}
                toast.showSuccess("充电桩验证成功")
            }
        }
        function onErrorOccurred(msg) {
            toast.showError("验证失败: " + msg)
        }
    }

    function startCharging() {
        if (scannedChargerId <= 0) {
            toast.showWarning("请先扫描有效的充电桩二维码")
            return
        }
        if (chargerData.status !== "online") {
            toast.showWarning("该充电桩不可用，当前状态: " + chargerData.status)
            return
        }
        if (net.connected && !isStarting) {
            isStarting = true
            if (selectedMode === 0) {
                net.startCharge(scannedChargerId, "auto", 0)
            } else if (selectedMode === 1) {
                net.startCharge(scannedChargerId, "duration", chargeDuration)
            } else {
                net.startCharge(scannedChargerId, "energy", chargeAmount)
            }
            toast.showInfo("正在启动充电...")
        }
    }

    function resetScan() {
        scanResult = ""
        isScanning = true
        hasResult = false
        scannedChargerId = 0
        chargerData = {}
        selectedMode = 0
        isStarting = false
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
                    text: "扫码充电"
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

                // ========== 扫码区域 ==========
                Rectangle {
                    width: parent.width
                    height: 280
                    color: "#1A1A1A"
                    
                    // 扫描框装饰
                    Column {
                        anchors.centerIn: parent
                        spacing: 0
                        
                        // 扫描框
                        Rectangle {
                            width: 220
                            height: 220
                            color: "transparent"

                            // 四角装饰
                            Rectangle {
                                width: 35
                                height: 4
                                color: "#4CAF50"
                                anchors.top: parent.top
                                anchors.left: parent.left
                            }
                            Rectangle {
                                width: 4
                                height: 35
                                color: "#4CAF50"
                                anchors.top: parent.top
                                anchors.left: parent.left
                            }
                            Rectangle {
                                width: 35
                                height: 4
                                color: "#4CAF50"
                                anchors.top: parent.top
                                anchors.right: parent.right
                            }
                            Rectangle {
                                width: 4
                                height: 35
                                color: "#4CAF50"
                                anchors.top: parent.top
                                anchors.right: parent.right
                            }
                            Rectangle {
                                width: 35
                                height: 4
                                color: "#4CAF50"
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                            }
                            Rectangle {
                                width: 4
                                height: 35
                                color: "#4CAF50"
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                            }
                            Rectangle {
                                width: 35
                                height: 4
                                color: "#4CAF50"
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                            }
                            Rectangle {
                                width: 4
                                height: 35
                                color: "#4CAF50"
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                            }

                            // 扫描线动画
                            Rectangle {
                                id: scanLine
                                width: parent.width - 20
                                height: 3
                                radius: 1.5
                                color: "#4CAF50"
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.topMargin: 10

                                SequentialAnimation on anchors.topMargin {
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 10; to: parent.height - 15; duration: 2000; easing.type: Easing.Linear }
                                    NumberAnimation { from: parent.height - 15; to: 10; duration: 2000; easing.type: Easing.Linear }
                                }
                            }
                        }
                    }

                    // 提示文字
                    Column {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 16
                        spacing: 4
                        
                        Text {
                            text: isScanning ? "将二维码放入框内，自动扫描" : "✅ 扫描成功"
                            color: isScanning ? "#CCCCCC" : "#4CAF50"
                            font.pixelSize: 14
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: isScanning ? "请确保光线充足" : "充电桩 #" + scannedChargerId + " 已识别"
                            color: "#888888"
                            font.pixelSize: 12
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: !isScanning
                        }
                    }
                }

                // ========== 手动输入区域 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "或手动输入充电桩编号"
                            font.bold: true
                            font.pixelSize: 14
                            color: "#333"
                        }

                        Row {
                            spacing: 12
                            width: parent.width

                            TextField {
                                id: txtManualId
                                placeholderText: "输入编号，如: 1"
                                font.pixelSize: 14
                                inputMethodHints: Qt.ImhDigitsOnly
                                width: (parent.width - 12) / 2
                                height: 44
                                background: Rectangle {
                                    color: "white"
                                    border.width: 1
                                    border.color: "#E0E0E0"
                                    radius: 8
                                }
                            }

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 44
                                radius: 8
                                color: "#4A90D9"

                                Text {
                                    text: "验证"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 14
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        var id = parseInt(txtManualId.text)
                                        if (id > 0) {
                                            scannedChargerId = id
                                            hasResult = true
                                            isScanning = false
                                            net.getChargerDetail(id)
                                            toast.show("正在验证充电桩...")
                                        } else {
                                            toast.showWarning("请输入有效的编号")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 扫描结果 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    visible: hasResult && scannedChargerId > 0
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "📍 充电桩信息"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }

                        Rectangle {
                            width: parent.width
                            radius: 16
                            color: "white"
                            
                            // 卡片阴影效果
                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius
                                color: "#000000"
                                opacity: 0.05
                                anchors.top: parent.top
                                anchors.topMargin: 4
                            }
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 16

                                Row {
                                    width: parent.width
                                    spacing: 12

                                    // 状态图标
                                    Rectangle {
                                        width: 60
                                        height: 60
                                        radius: 30
                                        color: chargerData.status === "online" ? "#E8F5E9" :
                                               chargerData.status === "charging" ? "#FFF3E0" : "#FFEBEE"
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: "⚡"
                                            font.pixelSize: 32
                                            anchors.centerIn: parent
                                            color: chargerData.status === "online" ? "#4CAF50" :
                                                   chargerData.status === "charging" ? "#FF9800" : "#E53935"
                                        }
                                    }

                                    Column {
                                        spacing: 6
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: chargerData.name || chargerData.serial_number || "充电桩 #" + scannedChargerId
                                            font.bold: true
                                            font.pixelSize: 18
                                            color: "#333"
                                        }

                                        Text {
                                            text: "📍 " + (chargerData.location || "位置未知")
                                            font.pixelSize: 13
                                            color: "#666"
                                        }
                                        
                                        // 状态标签
                                        Rectangle {
                                            height: 24
                                            radius: 12
                                            color: chargerData.status === "online" ? "#E8F5E9" :
                                                   chargerData.status === "charging" ? "#FFF3E0" : "#FFEBEE"
                                            
                                            Text {
                                                text: chargerData.status === "online" ? "  ✅ 空闲可用  " :
                                                      chargerData.status === "charging" ? "  ⚠️ 使用中  " :
                                                      "  ❌ 离线  "
                                                font.pixelSize: 12
                                                color: chargerData.status === "online" ? "#4CAF50" :
                                                       chargerData.status === "charging" ? "#FF9800" : "#E53935"
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }
                                    }
                                }

                                Grid {
                                    columns: 2
                                    spacing: 12
                                    width: parent.width

                                    Rectangle {
                                        width: (parent.width - 12) / 2
                                        height: 56
                                        color: "#F5F7FA"
                                        radius: 12
                                        Column {
                                            anchors.centerIn: parent
                                            Text {
                                                text: "⚡ " + (chargerData.power_kw || chargerData.power || 60) + " kW"
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#4A90D9"
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: "额定功率"
                                                font.pixelSize: 11
                                                color: "#999"
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }
                                    }

                                    Rectangle {
                                        width: (parent.width - 12) / 2
                                        height: 56
                                        color: "#F5F7FA"
                                        radius: 12
                                        Column {
                                            anchors.centerIn: parent
                                            Text {
                                                text: "💰 ¥" + ((chargerData.price_kwh || chargerData.price || 1.2)).toFixed(2) + "/度"
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#FF9800"
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: "电费单价"
                                                font.pixelSize: 11
                                                color: "#999"
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ========== 充电模式选择 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    visible: hasResult && chargerData.status === "online"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "⚡ 选择充电模式"
                            font.bold: true
                            font.pixelSize: 16
                            color: "#333"
                        }

                        // 模式1: 自动充满
                        Rectangle {
                            width: parent.width
                            height: 72
                            radius: 12
                            color: selectedMode === 0 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedMode === 0 ? 2 : 1
                            border.color: selectedMode === 0 ? "#4A90D9" : "#E0E0E0"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12
                                
                                Rectangle {
                                    width: 48
                                    height: 48
                                    radius: 24
                                    color: selectedMode === 0 ? "#4A90D9" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "⚡"
                                        font.pixelSize: 24
                                        color: "white"
                                        anchors.centerIn: parent
                                    }
                                }
                                
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "自动充满"
                                        font.bold: true
                                        font.pixelSize: 15
                                        color: "#333"
                                    }
                                    
                                    Text {
                                        text: "充到100%自动停止，智能控制"
                                        font.pixelSize: 12
                                        color: "#666"
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    color: selectedMode === 0 ? "#4A90D9" : "transparent"
                                    border.width: 2
                                    border.color: selectedMode === 0 ? "#4A90D9" : "#CCCCCC"
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: "✓"
                                        font.pixelSize: 14
                                        color: "white"
                                        anchors.centerIn: parent
                                        visible: selectedMode === 0
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedMode = 0
                            }
                        }

                        // 模式2: 定时停止
                        Rectangle {
                            width: parent.width
                            height: 72
                            radius: 12
                            color: selectedMode === 1 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedMode === 1 ? 2 : 1
                            border.color: selectedMode === 1 ? "#4A90D9" : "#E0E0E0"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                
                                Row {
                                    spacing: 12
                                    
                                    Rectangle {
                                        width: 48
                                        height: 48
                                        radius: 24
                                        color: selectedMode === 1 ? "#4A90D9" : "#CCCCCC"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "⏱"
                                            font.pixelSize: 24
                                            color: "white"
                                            anchors.centerIn: parent
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "定时停止"
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: "#333"
                                        }
                                        
                                        Text {
                                            text: "设置充电时长，准时停止"
                                            font.pixelSize: 12
                                            color: "#666"
                                        }
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                    
                                    Rectangle {
                                        width: 24
                                        height: 24
                                        radius: 12
                                        color: selectedMode === 1 ? "#4A90D9" : "transparent"
                                        border.width: 2
                                        border.color: selectedMode === 1 ? "#4A90D9" : "#CCCCCC"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "✓"
                                            font.pixelSize: 14
                                            color: "white"
                                            anchors.centerIn: parent
                                            visible: selectedMode === 1
                                        }
                                    }
                                }
                                
                                // 时间选择滑块
                                Row {
                                    spacing: 8
                                    anchors.leftMargin: 60
                                    
                                    Text {
                                        text: "30分钟"
                                        font.pixelSize: 11
                                        color: "#999"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    Slider {
                                        id: durationSlider
                                        from: 0.5
                                        to: 4
                                        value: chargeDuration / 60
                                        stepSize: 0.5
                                        width: 150
                                        anchors.verticalCenter: parent.verticalCenter
                                        onValueChanged: {
                                            chargeDuration = Math.round(value * 60)
                                        }
                                    }
                                    
                                    Text {
                                        text: Math.round(chargeDuration) + "分钟"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#4A90D9"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedMode = 1
                            }
                        }

                        // 模式3: 按电量
                        Rectangle {
                            width: parent.width
                            height: 72
                            radius: 12
                            color: selectedMode === 2 ? "#E3F2FD" : "#F5F7FA"
                            border.width: selectedMode === 2 ? 2 : 1
                            border.color: selectedMode === 2 ? "#4A90D9" : "#E0E0E0"
                            
                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                
                                Row {
                                    spacing: 12
                                    
                                    Rectangle {
                                        width: 48
                                        height: 48
                                        radius: 24
                                        color: selectedMode === 2 ? "#4A90D9" : "#CCCCCC"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "🔋"
                                            font.pixelSize: 24
                                            color: "white"
                                            anchors.centerIn: parent
                                        }
                                    }
                                    
                                    Column {
                                        spacing: 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "按电量充"
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: "#333"
                                        }
                                        
                                        Text {
                                            text: "设置充电度数，到量停止"
                                            font.pixelSize: 12
                                            color: "#666"
                                        }
                                    }
                                    
                                    Item { Layout.fillWidth: true }
                                    
                                    Rectangle {
                                        width: 24
                                        height: 24
                                        radius: 12
                                        color: selectedMode === 2 ? "#4A90D9" : "transparent"
                                        border.width: 2
                                        border.color: selectedMode === 2 ? "#4A90D9" : "#CCCCCC"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "✓"
                                            font.pixelSize: 14
                                            color: "white"
                                            anchors.centerIn: parent
                                            visible: selectedMode === 2
                                        }
                                    }
                                }
                                
                                // 电量选择
                                Row {
                                    spacing: 8
                                    anchors.leftMargin: 60
                                    
                                    Text {
                                        text: "10度"
                                        font.pixelSize: 11
                                        color: "#999"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    Slider {
                                        id: amountSlider
                                        from: 10
                                        to: 100
                                        value: chargeAmount
                                        stepSize: 5
                                        width: 150
                                        anchors.verticalCenter: parent.verticalCenter
                                        onValueChanged: {
                                            chargeAmount = Math.round(value)
                                        }
                                    }
                                    
                                    Text {
                                        text: Math.round(chargeAmount) + "度"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#4A90D9"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedMode = 2
                            }
                        }
                    }
                }

                // ========== 预估费用展示 ==========
                Rectangle {
                    width: parent.width
                    color: "#4A90D9"
                    visible: hasResult && chargerData.status === "online"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        
                        Text {
                            text: "💡 预估信息"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }
                        
                        Row {
                            spacing: 24
                            
                            Column {
                                spacing: 2
                                
                                Text {
                                    text: "预计充电"
                                    color: "#BBDEFB"
                                    font.pixelSize: 12
                                }
                                
                                Text {
                                    text: getEstimatedKwh().toFixed(1) + " 度"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                }
                            }
                            
                            Column {
                                spacing: 2
                                
                                Text {
                                    text: "预计费用"
                                    color: "#BBDEFB"
                                    font.pixelSize: 12
                                }
                                
                                Text {
                                    text: "¥" + getEstimatedCost()
                                    color: "#FFD700"
                                    font.bold: true
                                    font.pixelSize: 18
                                }
                            }
                            
                            Column {
                                spacing: 2
                                
                                Text {
                                    text: "预计时间"
                                    color: "#BBDEFB"
                                    font.pixelSize: 12
                                }
                                
                                Text {
                                    text: selectedMode === 1 ? Math.round(chargeDuration) + "分钟" : 
                                          (getEstimatedKwh() / (chargerData.power_kw || chargerData.power || 60) * 60).toFixed(0) + "分钟"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                }
                            }
                        }
                    }
                }

                // ========== 底部按钮 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        // 充电前提示
                        Rectangle {
                            width: parent.width
                            radius: 8
                            color: "#FFF8E1"
                            
                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8
                                
                                Text {
                                    text: "💡"
                                    font.pixelSize: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                
                                Text {
                                    text: "请确保充电枪已连接车辆后再开始充电"
                                    font.pixelSize: 12
                                    color: "#FF9800"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }

                        // 开始充电按钮
                        Rectangle {
                            width: parent.width
                            height: 54
                            radius: 27
                            color: hasResult && chargerData.status === "online" && !isStarting ? "#4CAF50" : "#CCCCCC"
                            
                            Row {
                                anchors.centerIn: parent
                                spacing: 8
                                
                                Text {
                                    text: isStarting ? "正在启动..." : "⚡ 开始充电"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 17
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                
                                // 加载动画
                                Rectangle {
                                    visible: isStarting
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
                                        running: isStarting
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: hasResult && chargerData.status === "online" && !isStarting
                                onClicked: {
                                    // 确认对话框
                                    confirmationDialog.visible = true
                                }
                            }
                        }

                        // 重新扫描按钮
                        Row {
                            spacing: 12
                            width: parent.width

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 44
                                radius: 22
                                color: "white"
                                border.width: 1.5
                                border.color: "#4A90D9"

                                Text {
                                    text: "重新扫描"
                                    color: "#4A90D9"
                                    font.bold: true
                                    font.pixelSize: 14
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: resetScan()
                                }
                            }

                            Rectangle {
                                width: (parent.width - 12) / 2
                                height: 44
                                radius: 22
                                color: "#FF9800"

                                Text {
                                    text: "📅 预约充电"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 14
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.stackView.push(reservationPageComponent, { "chargerId": scannedChargerId })
                                    }
                                }
                            }
                        }
                    }
                }

                Item { height: 100 }
            }
        }
        
        // 确认对话框
        Rectangle {
            id: confirmationDialog
            visible: false
            anchors.fill: parent
            color: "#80000000"
            
            Rectangle {
                width: parent.width - 48
                height: 240
                radius: 16
                color: "white"
                anchors.centerIn: parent
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 16
                    
                    Text {
                        text: "⚡ 确认开始充电"
                        font.bold: true
                        font.pixelSize: 18
                        color: "#333"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Column {
                        spacing: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                        
                        Text {
                            text: "充电桩: " + (chargerData.name || ("#" + scannedChargerId))
                            font.pixelSize: 14
                            color: "#666"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "充电模式: " + (selectedMode === 0 ? "自动充满" : 
                                                  selectedMode === 1 ? "定时" + chargeDuration + "分钟" : 
                                                  "按电量" + chargeAmount + "度")
                            font.pixelSize: 14
                            color: "#666"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "预估费用: ¥" + getEstimatedCost()
                            font.bold: true
                            font.pixelSize: 16
                            color: "#FF9800"
                            anchors.horizontalCenter: parent.horizontalCenter
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
                            height: 44
                            radius: 22
                            color: "#F5F7FA"
                            
                            Text {
                                text: "取消"
                                color: "#666"
                                font.bold: true
                                font.pixelSize: 14
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: confirmationDialog.visible = false
                            }
                        }
                        
                        Rectangle {
                            width: (parent.width - 12) / 2
                            height: 44
                            radius: 22
                            color: "#4CAF50"
                            
                            Text {
                                text: "确认开始"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                                anchors.centerIn: parent
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    confirmationDialog.visible = false
                                    startCharging()
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
