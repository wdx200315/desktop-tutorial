import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "首页"
    property var chargerList: ListModel { }
    property bool isLoading: false
    property string userName: "用户"
    property double userBalance: 0.00
    property bool hasError: false
    property string errorMessage: ""
    property bool isCharging: false
    property var currentChargeSession: ({})

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (!net.connected) {
            toast.showWarning("网络未连接，请检查网络后重试")
            hasError = true
            errorMessage = "网络连接失败"
            return
        }
        isLoading = true
        hasError = false
        errorMessage = ""
        net.getChargerList(1, 20, "all")
        net.getUserInfo()
        // 检查是否有正在进行的充电
        net.getChargeMonitor()
    }

    function refresh() {
        loadData()
    }

    function getAvailableCount() {
        var count = 0
        for (var i = 0; i < chargerList.count; i++) {
            if (chargerList.get(i).status === "online") count++
        }
        return count
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "2001" && resp.status === "ok") {
                chargerList.clear()
                var list = resp.data || []
                for (var i = 0; i < list.length; ++i) {
                    chargerList.append(list[i])
                }
                if (chargerList.count === 0) {
                    toast.showInfo("暂无可用充电桩")
                }
            }
            if (resp.cmd === "1005" && resp.status === "ok") {
                userName = resp.data.username || "用户"
                userBalance = parseFloat(resp.data.balance) || 0
                // 检查余额是否充足
                if (userBalance < 10 && userBalance > 0) {
                    toast.showWarning("余额不足，建议及时充值")
                }
            }
            // 充电监控响应
            if (resp.cmd === "3003" && resp.status === "ok") {
                var chargeData = resp.data || {}
                if (chargeData.charger_id && chargeData.status === "charging") {
                    isCharging = true
                    currentChargeSession = chargeData
                } else {
                    isCharging = false
                    currentChargeSession = {}
                }
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            hasError = true
            errorMessage = msg
            toast.showError("加载失败: " + msg)
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // ========== 顶部状态栏 ==========
        Rectangle {
            width: parent.width
            height: 140
            color: "#4A90D9"

            // 背景装饰圆
            Rectangle {
                width: 200
                height: 200
                radius: 100
                color: "#FFFFFF"
                opacity: 0.05
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: -80
                anchors.rightMargin: -60
            }

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 6

                // 第一行：位置 + 通知
                Row {
                    width: parent.width
                    spacing: 8

                    Row {
                        spacing: 4
                        Text { text: "📍"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            text: "北京市朝阳区"
                            color: "white"
                            font.pixelSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // 通知徽章
                    Rectangle {
                        width: 40
                        height: 40
                        radius: 20
                        color: "#FFFFFF22"
                        
                        Text {
                            text: "🔔"
                            font.pixelSize: 18
                            anchors.centerIn: parent
                        }
                        
                        // 消息数量徽章（如果有新消息）
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: "#FF5252"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 6
                            anchors.rightMargin: 6
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(messagePageComponent)
                        }
                    }
                }

                // 第二行：问候语
                Text {
                    text: "你好，" + userName
                    color: "white"
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    text: "准备好充电了吗？"
                    color: "#BBDEFB"
                    font.pixelSize: 13
                }

                // 第三行：余额显示
                Row {
                    spacing: 4
                    Text {
                        text: "💰"
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "¥" + userBalance.toFixed(2)
                        color: "#FFD700"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: userBalance < 10 ? " ⚠️ 余额不足" : ""
                        color: "#FF5252"
                        font.pixelSize: 12
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                        visible: userBalance < 10 && userBalance > 0
                    }

                    Rectangle {
                        width: 1
                        height: 14
                        color: "#FFFFFF66"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                    }

                    Rectangle {
                        height: 24
                        radius: 12
                        color: "#FFFFFF33"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        
                        Text {
                            text: " 充值 "
                            color: "white"
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(rechargePageComponent)
                        }
                    }
                }
            }
        }

        // ========== 正在充电状态卡片 ==========
        Rectangle {
            width: parent.width
            height: isCharging ? 100 : 0
            color: "#4CAF50"
            clip: true
            Behavior on height { NumberAnimation { duration: 300 } }
            
            visible: isCharging
            
            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4
                
                Row {
                    width: parent.width
                    spacing: 12
                    
                    // 充电动画图标
                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: "#FFFFFF33"
                        
                        Text {
                            text: "⚡"
                            font.pixelSize: 28
                            color: "white"
                            anchors.centerIn: parent
                        }
                        
                        // 脉冲动画
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + 10
                            height: parent.height + 10
                            radius: width / 2
                            color: "#FFFFFF"
                            opacity: 0.3
                            
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.3; to: 0; duration: 1000 }
                                PauseAnimation { duration: 1000 }
                            }
                        }
                    }
                    
                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter
                        
                        Text {
                            text: "正在充电中"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                        }
                        
                        Text {
                            text: "充电桩 #" + (currentChargeSession.charger_id || "-") + 
                                  " | 已充 " + (currentChargeSession.duration_minutes || 0) + " 分钟"
                            color: "#BBDEFB"
                            font.pixelSize: 12
                        }
                        
                        Text {
                            text: "已充 " + (currentChargeSession.energy_kwh || 0).toFixed(1) + " 度 | 预计费用 ¥" + (currentChargeSession.estimated_cost || 0).toFixed(2)
                            color: "#BBDEFB"
                            font.pixelSize: 12
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Rectangle {
                        width: 80
                        height: 36
                        radius: 18
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                        
                        Text {
                            text: "查看详情"
                            color: "#4CAF50"
                            font.bold: true
                            font.pixelSize: 12
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(chargeMonitorPageComponent)
                        }
                    }
                }
            }
        }

        // ========== 搜索栏 ==========
        Rectangle {
            width: parent.width
            height: 50
            color: "#F5F7FA"
            Rectangle {
                width: parent.width - 32
                height: 36
                radius: 18
                color: "white"
                anchors.centerIn: parent

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    spacing: 8
                    verticalCenter: parent.verticalCenter

                    Text {
                        text: "🔍"
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: "搜索充电桩名称或位置..."
                        color: "#999999"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: toast.showInfo("搜索功能开发中")
                }
            }
        }

        // ========== 核心功能区 ==========
        Rectangle {
            width: parent.width
            color: "white"
            
            // 顶部装饰线
            Rectangle {
                width: parent.width
                height: 3
                color: "#4A90D9"
            }
            
            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // 第一行大按钮 - 扫码充电（主入口）
                Row {
                    width: parent.width
                    spacing: 12

                    // 扫码充电 - 主按钮（更大更醒目）
                    Rectangle {
                        width: (parent.width - 12) / 2
                        height: 100
                        radius: 16
                        
                        // 渐变背景
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#4A90D9" }
                            GradientStop { position: 1.0; color: "#2979FF" }
                        }
                        
                        // 光泽效果
                        Rectangle {
                            width: parent.width
                            height: parent.height / 2
                            radius: parent.radius
                            color: "#FFFFFF"
                            opacity: 0.1
                            anchors.top: parent.top
                        }
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 2

                            Row {
                                spacing: 8
                                Text {
                                    text: "⚡"
                                    font.pixelSize: 32
                                }
                                Text {
                                    text: "扫码充电"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Text {
                                text: "扫描充电桩二维码"
                                color: "#BBDEFB"
                                font.pixelSize: 12
                            }
                            
                            Text {
                                text: "快速开始充电"
                                color: "#BBDEFB"
                                font.pixelSize: 12
                            }

                            Rectangle {
                                width: 90
                                height: 28
                                radius: 14
                                color: "white"
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottomMargin: -2
                                
                                Text {
                                    text: "立即使用 →"
                                    color: "#4A90D9"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.centerIn: parent
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(scanPageComponent)
                        }
                    }

                    // 预约充电
                    Rectangle {
                        width: (parent.width - 12) / 2
                        height: 100
                        radius: 16
                        
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#FF9800" }
                            GradientStop { position: 1.0; color: "#FF6D00" }
                        }
                        
                        Column {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 2

                            Row {
                                spacing: 8
                                Text {
                                    text: "📅"
                                    font.pixelSize: 32
                                }
                                Text {
                                    text: "预约充电"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 18
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Text {
                                text: "提前预约充电桩"
                                color: "#FFE0B2"
                                font.pixelSize: 12
                            }
                            
                            Text {
                                text: "避免等待"
                                color: "#FFE0B2"
                                font.pixelSize: 12
                            }

                            Rectangle {
                                width: 70
                                height: 28
                                radius: 14
                                color: "white"
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottomMargin: -2
                                
                                Text {
                                    text: "去预约 →"
                                    color: "#FF9800"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.centerIn: parent
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(reservationPageComponent)
                        }
                    }
                }
                
                // 附近可用充电桩数量提示
                Row {
                    width: parent.width
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#4CAF50"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 1
                    }
                    
                    Text {
                        text: "附近有 " + getAvailableCount() + " 个充电桩可用"
                        font.pixelSize: 13
                        color: "#666"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // 第二行小按钮
                Row {
                    width: parent.width
                    spacing: 12

                    Rectangle {
                        width: (parent.width - 36) / 4
                        height: 76
                        radius: 12
                        color: "#E8F5E9"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "📋"; font.pixelSize: 26; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "我的订单"; font.pixelSize: 11; color: "#333"; anchors.horizontalCenter: parent.horizontalCenter }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(orderPageComponent)
                        }
                    }

                    Rectangle {
                        width: (parent.width - 36) / 4
                        height: 76
                        radius: 12
                        color: "#E3F2FD"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "🎫"; font.pixelSize: 26; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "优惠券"; font.pixelSize: 11; color: "#333"; anchors.horizontalCenter: parent.horizontalCenter }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(couponPageComponent)
                        }
                    }

                    Rectangle {
                        width: (parent.width - 36) / 4
                        height: 76
                        radius: 12
                        color: "#FFF3E0"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "💰"; font.pixelSize: 26; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "充值"; font.pixelSize: 11; color: "#333"; anchors.horizontalCenter: parent.horizontalCenter }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(rechargePageComponent)
                        }
                    }

                    Rectangle {
                        width: (parent.width - 36) / 4
                        height: 76
                        radius: 12
                        color: "#F3E5F5"

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "👑"; font.pixelSize: 26; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: "会员"; font.pixelSize: 11; color: "#333"; anchors.horizontalCenter: parent.horizontalCenter }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(memberPageComponent)
                        }
                    }
                }
            }
        }

        // ========== 充电桩列表区域 ==========
        Column {
            width: parent.width
            spacing: 0
            
            // 间隔
            Rectangle {
                width: parent.width
                height: 10
                color: "#F5F7FA"
            }

            // 标题栏
            Row {
                width: parent.width
                anchors.margins: 16
                spacing: 8

                Text {
                    text: "附近充电桩"
                    font.bold: true
                    font.pixelSize: 16
                    color: "#333"
                }
                
                // 可用数量标签
                Rectangle {
                    height: 22
                    radius: 11
                    color: "#E8F5E9"
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        text: " " + chargerList.count + " 个 "
                        font.pixelSize: 12
                        color: "#4CAF50"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item { Layout.fillWidth: true }

                // 刷新按钮
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: "#F5F7FA"
                    
                    Text {
                        text: "🔄"
                        font.pixelSize: 14
                        color: "#666"
                        anchors.centerIn: parent
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: refresh()
                    }
                }
            }

            // 加载状态
            LoadingOverlay {
                visible: isLoading
                loading: isLoading
                loadingText: "加载充电桩中..."
                width: parent.width
                height: 150
            }

            // 错误状态
            EmptyState {
                visible: hasError
                emptyIcon: "❌"
                emptyTitle: "加载失败"
                emptyDescription: errorMessage
                emptyActionText: "重试"
                emptyAction: function() {
                    refresh()
                }
                width: parent.width
                height: 150
            }

            // 空状态
            EmptyState {
                visible: !isLoading && !hasError && chargerList.count === 0
                emptyIcon: "🔌"
                emptyTitle: "暂无充电桩"
                emptyDescription: "附近没有可用的充电桩"
                width: parent.width
                height: 150
            }

            // 充电桩列表
            ListView {
                visible: chargerList.count > 0
                width: parent.width
                height: Math.min(chargerList.count * 120, 360)
                model: chargerList
                clip: true
                spacing: 0

                delegate: Rectangle {
                    width: parent.width
                    height: 120
                    color: "white"

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#F0F0F0"
                        anchors.bottom: parent.bottom
                    }

                    // 状态标签
                    Rectangle {
                        width: 56
                        height: 22
                        radius: 11
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 12
                        anchors.rightMargin: 12
                        color: model.status === "online" ? "#E8F5E9" :
                               model.status === "charging" ? "#FFF3E0" : "#FFEBEE"

                        Text {
                            text: model.status === "online" ? "空闲" :
                                  model.status === "charging" ? "使用中" : "离线"
                            font.pixelSize: 11
                            color: model.status === "online" ? "#4CAF50" :
                                   model.status === "charging" ? "#FF9800" : "#E53935"
                            anchors.centerIn: parent
                        }
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        // 充电桩名称
                        Row {
                            width: parent.width
                            Text {
                                text: "⚡ " + (model.name || ("充电桩 #" + model.id))
                                font.bold: true
                                font.pixelSize: 15
                                color: "#333"
                            }
                        }

                        // 位置
                        Row {
                            width: parent.width
                            Text {
                                text: "📍 " + (model.location || "位置未知")
                                font.pixelSize: 12
                                color: "#666"
                            }
                        }

                        // 功率和价格
                        Row {
                            width: parent.width
                            spacing: 16

                            Text {
                                text: "⚡ " + (model.power_kw || model.power || 60) + " kW"
                                font.pixelSize: 13
                                color: "#4A90D9"
                                font.bold: true
                            }

                            Text {
                                text: "💰 ¥" + ((model.price_kwh || model.price || 1.2)).toFixed(2) + "/度"
                                font.pixelSize: 13
                                color: "#FF9800"
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                width: 70
                                height: 30
                                radius: 15
                                color: model.status === "online" ? "#4CAF50" : "#CCCCCC"

                                Text {
                                    text: model.status === "online" ? "去充电" : "不可用"
                                    color: "white"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: model.status === "online"
                                    onClicked: {
                                        if (model.status === "online") {
                                            root.stackView.push(chargerDetailPageComponent, { "chargerId": model.id })
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                footer: Item { height: 80 }
            }
        }

        // 底部间距
        Item { height: 100 }
    }

    header: null
}
