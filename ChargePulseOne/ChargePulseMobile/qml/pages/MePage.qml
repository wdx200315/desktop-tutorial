import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "我的"
    property string username: "未登录"
    property string phone: "138****8888"
    property string memberLevel: "黄金会员"
    property double balance: 156.80
    property int orderCount: 23
    property int couponCount: 5
    property double totalEnergy: 1256.8  // 总充电量
    property double carbonReduction: 1256.8 * 0.997  // 碳减排量(kg)
    property bool showLogoutConfirm: false
    
    // 会员等级配置
    property var memberConfig: ({
        current: "黄金会员",
        next: "铂金会员",
        progress: 0.65,  // 65%
        points: 3250,
        nextPoints: 5000,
        benefits: ["9.5折充电", "优先客服", "专属活动"]
    })

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            net.getUserInfo()
            net.getMemberStats()
        }
    }

    function refresh() {
        loadData()
    }

    function logout() {
        session.clear()
        root.stackView.clear()
        root.stackView.push(loginPageComponent)
        toast.showInfo("已退出登录")
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "1005" && resp.status === "ok") {
                username = resp.data.username || username
                balance = parseFloat(resp.data.balance) || balance
            }
            if (resp.cmd === "9202" && resp.status === "ok") {
                totalEnergy = resp.data.total_energy || totalEnergy
                carbonReduction = resp.data.carbon_reduction || carbonReduction
            }
        }
        function onErrorOccurred(msg) {
            // 使用模拟数据
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // ========== 顶部用户信息区 ==========
        Rectangle {
            width: parent.width
            height: 220
            color: "#4A90D9"
            
            // 背景装饰
            Rectangle {
                width: 300
                height: 300
                radius: 150
                color: "#FFFFFF"
                opacity: 0.05
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: -100
                anchors.rightMargin: -100
            }

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16

                // 第一行：头像 + 用户信息
                Row {
                    width: parent.width
                    spacing: 16

                    // 头像（带边框）
                    Rectangle {
                        width: 76
                        height: 76
                        radius: 38
                        color: "white"
                        
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            color: "transparent"
                            border.width: 3
                            border.color: "#FFD700"
                        }

                        Text {
                            anchors.centerIn: parent
                            text: (username && username.length > 0 ? username[0] : "U").toUpperCase()
                            font.pixelSize: 36
                            font.bold: true
                            color: "#4A90D9"
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        Text {
                            text: username
                            color: "white"
                            font.pixelSize: 22
                            font.bold: true
                        }

                        Text {
                            text: phone
                            color: "#BBDEFB"
                            font.pixelSize: 13
                        }

                        // 会员等级和积分
                        Row {
                            spacing: 8
                            
                            Rectangle {
                                height: 24
                                radius: 12
                                color: "#FFD700"
                                
                                Text {
                                    text: " 👑 " + memberLevel + " "
                                    color: "#333"
                                    font.pixelSize: 12
                                    font.bold: true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            
                            Rectangle {
                                height: 24
                                radius: 12
                                color: "#FFFFFF33"
                                
                                Text {
                                    text: " ⭐ " + memberConfig.points + " 积分 "
                                    color: "white"
                                    font.pixelSize: 11
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    // 设置按钮
                    Rectangle {
                        width: 36
                        height: 36
                        radius: 18
                        color: "#FFFFFF22"
                        
                        Text {
                            text: "⚙️"
                            font.pixelSize: 18
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(settingsPageComponent)
                        }
                    }
                }

                // 第二行：会员成长进度
                Rectangle {
                    width: parent.width
                    height: 60
                    radius: 12
                    color: "#FFFFFF22"
                    
                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        
                        Row {
                            width: parent.width
                            
                            Text {
                                text: "🌟 " + memberLevel
                                color: "#FFD700"
                                font.bold: true
                                font.pixelSize: 13
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Text {
                                text: memberConfig.progress * 100 + "%"
                                color: "#BBDEFB"
                                font.pixelSize: 12
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "距离" + memberConfig.next + "还需 " + (memberConfig.nextPoints - memberConfig.points) + "积分"
                                color: "#BBDEFB"
                                font.pixelSize: 11
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        
                        // 进度条
                        Rectangle {
                            width: parent.width
                            height: 6
                            radius: 3
                            color: "#FFFFFF33"
                            
                            Rectangle {
                                width: parent.width * memberConfig.progress
                                height: parent.height
                                radius: parent.radius
                                color: "#FFD700"
                            }
                        }
                    }
                }
                
                // 第三行：余额和充值
                Row {
                    width: parent.width
                    spacing: 12

                    // 余额卡片
                    Rectangle {
                        width: (parent.width - 12) / 2
                        height: 56
                        radius: 12
                        color: "#FFFFFF33"

                        Row {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "💰"
                                font.pixelSize: 24
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2

                                Text {
                                    text: "账户余额"
                                    color: "#BBDEFB"
                                    font.pixelSize: 11
                                }

                                Text {
                                    text: "¥" + balance.toFixed(2)
                                    color: "white"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(rechargePageComponent)
                        }
                    }

                    // 充值按钮
                    Rectangle {
                        width: (parent.width - 12) / 2
                        height: 56
                        radius: 12
                        color: "#FFD700"

                        Text {
                            anchors.centerIn: parent
                            text: "⚡ 立即充值"
                            color: "#333"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.stackView.push(rechargePageComponent)
                        }
                    }
                }
            }
        }

        Flickable {
            width: parent.width
            height: parent.height - 220
            contentHeight: contentColumn.height + 100
            clip: true

            Column {
                id: contentColumn
                spacing: 0
                
                // ========== 环保贡献数据 ==========
                Rectangle {
                    width: parent.width
                    color: "#E8F5E9"
                    
                    Row {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 0
                        
                        // 充电量
                        Column {
                            spacing: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: (parent.width - 32) / 3
                            
                            Text {
                                text: "⚡"
                                font.pixelSize: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: totalEnergy.toFixed(1)
                                color: "#333"
                                font.bold: true
                                font.pixelSize: 18
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: "累计充电(度)"
                                color: "#666"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                        
                        Rectangle {
                            width: 1
                            height: 40
                            color: "#C8E6C9"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        // 碳减排
                        Column {
                            spacing: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: (parent.width - 32) / 3
                            
                            Text {
                                text: "🌱"
                                font.pixelSize: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: carbonReduction.toFixed(1)
                                color: "#333"
                                font.bold: true
                                font.pixelSize: 18
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: "碳减排(kg)"
                                color: "#666"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                        
                        Rectangle {
                            width: 1
                            height: 40
                            color: "#C8E6C9"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        // 节省费用
                        Column {
                            spacing: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: (parent.width - 32) / 3
                            
                            Text {
                                text: "💰"
                                font.pixelSize: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: "¥" + (totalEnergy * 0.5).toFixed(0)
                                color: "#333"
                                font.bold: true
                                font.pixelSize: 18
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            
                            Text {
                                text: "节省费用(元)"
                                color: "#666"
                                font.pixelSize: 11
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }

                // ========== 快捷服务入口 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        Text {
                            text: "我的服务"
                            font.bold: true
                            font.size: 14
                            color: "#333"
                        }

                        Row {
                            width: parent.width
                            spacing: 12

                            // 我的订单
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 85
                                radius: 12
                                color: "#E8F5E9"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6

                                    Text {
                                        text: "📋"
                                        font.pixelSize: 26
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "我的订单"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: orderCount + " 单"
                                        font.pixelSize: 10
                                        color: "#4CAF50"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(orderPageComponent)
                                }
                            }

                            // 预约记录
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 85
                                radius: 12
                                color: "#E3F2FD"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6

                                    Text {
                                        text: "📅"
                                        font.pixelSize: 26
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "预约记录"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "查看预约"
                                        font.pixelSize: 10
                                        color: "#4A90D9"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(reservationPageComponent)
                                }
                            }

                            // 我的优惠券
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 85
                                radius: 12
                                color: "#FFF3E0"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6

                                    Text {
                                        text: "🎫"
                                        font.pixelSize: 26
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "优惠券"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: couponCount + " 张可用"
                                        font.pixelSize: 10
                                        color: "#FF9800"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(couponPageComponent)
                                }
                            }

                            // 充电统计
                            Rectangle {
                                width: (parent.width - 36) / 4
                                height: 85
                                radius: 12
                                color: "#F3E5F5"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6

                                    Text {
                                        text: "📊"
                                        font.pixelSize: 26
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "充电统计"
                                        font.pixelSize: 12
                                        color: "#333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    
                                    Text {
                                        text: "查看详情"
                                        font.pixelSize: 10
                                        color: "#9C27B0"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(statisticsPageComponent)
                                }
                            }
                        }
                    }
                }

                // ========== 我的车辆 ==========
                Rectangle {
                    width: parent.width
                    color: "#F5F7FA"
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Row {
                            width: parent.width
                            spacing: 12

                            Rectangle {
                                width: parent.width
                                height: 70
                                radius: 12
                                color: "white"

                                Row {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 12

                                    Rectangle {
                                        width: 44
                                        height: 44
                                        radius: 22
                                        color: "#E3F2FD"
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: "🚗"
                                            font.pixelSize: 24
                                            anchors.centerIn: parent
                                        }
                                    }

                                    Column {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 4

                                        Text {
                                            text: "我的车辆"
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: "#333"
                                        }

                                        Text {
                                            text: "特斯拉 Model 3 | 京A·D12345"
                                            font.pixelSize: 12
                                            color: "#666"
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: ">"
                                        font.pixelSize: 20
                                        color: "#CCCCCC"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.stackView.push(vehiclePageComponent)
                                }
                            }
                        }
                    }
                }

                // ========== 功能菜单 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        width: parent.width
                        anchors.margins: 16
                        spacing: 0

                        // 个人信息
                        MenuItem {
                            menuIcon: "👤"
                            menuTitle: "个人信息"
                            menuSubtitle: "修改昵称、头像等信息"
                            onClicked: root.stackView.push(profilePageComponent)
                        }
                        
                        DividerLine {}

                        // 会员中心
                        MenuItem {
                            menuIcon: "👑"
                            menuTitle: "会员中心"
                            menuSubtitle: memberLevel
                            onClicked: root.stackView.push(memberPageComponent)
                        }
                        
                        DividerLine {}

                        // 费率说明
                        MenuItem {
                            menuIcon: "💰"
                            menuTitle: "费率说明"
                            menuSubtitle: "了解充电费用计算规则"
                            onClicked: root.stackView.push(ratePageComponent)
                        }
                        
                        DividerLine {}

                        // 告警记录
                        MenuItem {
                            menuIcon: "🔔"
                            menuTitle: "告警记录"
                            menuSubtitle: "查看设备异常提醒"
                            onClicked: root.stackView.push(alarmListPageComponent)
                        }
                    }
                }
                
                // 间隔
                Item { height: 12 }

                // ========== 其他设置 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    Column {
                        width: parent.width
                        anchors.margins: 16
                        spacing: 0

                        // 帮助中心
                        MenuItem {
                            menuIcon: "❓"
                            menuTitle: "帮助中心"
                            menuSubtitle: "常见问题解答"
                            onClicked: toast.showInfo("帮助中心功能开发中")
                        }
                        
                        DividerLine {}

                        // 设置
                        MenuItem {
                            menuIcon: "⚙️"
                            menuTitle: "设置"
                            menuSubtitle: "通知、隐私等"
                            onClicked: root.stackView.push(settingsPageComponent)
                        }
                    }
                }
                
                // 间隔
                Item { height: 12 }
                
                // ========== 退出登录 ==========
                Rectangle {
                    width: parent.width
                    color: "white"
                    anchors.margins: 16
                    
                    Rectangle {
                        width: parent.width
                        height: 50
                        radius: 12
                        color: "#FFEBEE"
                        
                        Text {
                            text: "退出登录"
                            color: "#E53935"
                            font.bold: true
                            font.pixelSize: 15
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: showLogoutConfirm = true
                        }
                    }
                }

                // ========== 底部间距 ==========
                Item { height: 20 }
            }
        }
    }
    
    // 退出登录确认
    Rectangle {
        visible: showLogoutConfirm
        anchors.fill: parent
        color: "#80000000"
        
        Rectangle {
            width: parent.width - 48
            height: 200
            radius: 16
            color: "white"
            anchors.centerIn: parent
            
            Column {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16
                
                Text {
                    text: "确认退出登录？"
                    font.bold: true
                    font.pixelSize: 18
                    color: "#333"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "退出后将返回登录页面"
                    font.pixelSize: 14
                    color: "#666"
                    anchors.horizontalCenter: parent.horizontalCenter
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
                            onClicked: showLogoutConfirm = false
                        }
                    }
                    
                    Rectangle {
                        width: (parent.width - 12) / 2
                        height: 48
                        radius: 24
                        color: "#E53935"
                        
                        Text {
                            text: "确认退出"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 15
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                showLogoutConfirm = false
                                logout()
                            }
                        }
                    }
                }
            }
        }
    }
    
    header: null
    
    // 辅助组件
    component MenuItem: Rectangle {
        width: parent.width
        height: 64
        color: "transparent"
        
        Row {
            anchors.fill: parent
            spacing: 12
            
            Rectangle {
                width: 40
                height: 40
                radius: 20
                color: "#F5F7FA"
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: menuIcon
                    font.pixelSize: 18
                    anchors.centerIn: parent
                }
            }
            
            Column {
                spacing: 2
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: menuTitle
                    font.pixelSize: 15
                    color: "#333"
                }
                
                Text {
                    text: menuSubtitle
                    font.pixelSize: 12
                    color: "#999"
                }
            }
            
            Item { Layout.fillWidth: true }
            
            Text {
                text: ">"
                font.pixelSize: 18
                color: "#CCCCCC"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // 属性必须在使用前定义
        property string menuIcon: ""
        property string menuTitle: ""
        property string menuSubtitle: ""
        property var clickHandler: null

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (clickHandler) clickHandler.clicked()
            }
        }
    }

    component DividerLine: Rectangle {
        width: parent.width - 52
        height: 1
        color: "#F0F0F0"
        anchors.leftMargin: 52
    }

    // 刷新按钮
    Rectangle {
        width: 44
        height: 44
        radius: 22
        color: "#4A90D9"
        anchors.right: parent.right
        anchors.bottomMargin: 80
        anchors.rightMargin: 16
        
        Text {
            text: "🔄"
            font.pixelSize: 18
            anchors.centerIn: parent
        }
        
        MouseArea {
            anchors.fill: parent
            onClicked: refresh()
        }
    }
}
