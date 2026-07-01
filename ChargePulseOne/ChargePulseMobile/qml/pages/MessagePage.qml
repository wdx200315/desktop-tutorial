import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "消息中心"
    property var messageModel: ListModel { }
    property bool isLoading: false
    property int unreadCount: 0
    property int selectedTab: 0  // 0=全部, 1=系统, 2=订单, 3=活动
    property string searchText: ""
    property bool showSearch: false

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            isLoading = true
            net.getMessageList(1, 50)
        } else {
            toast.showWarning("网络未连接")
        }
    }

    function refresh() {
        loadData()
    }

    function markAsRead(msgId) {
        net.readMessage(msgId)
        toast.showSuccess("消息已标记为已读")
    }

    function markAllAsRead() {
        for (var i = 0; i < messageModel.count; i++) {
            var msg = messageModel.get(i)
            if (msg.is_read === 0 || msg.is_read === false) {
                net.readMessage(msg.id)
            }
        }
        unreadCount = 0
        toast.showSuccess("全部消息已标记为已读")
    }

    function deleteMessage(msgId) {
        net.deleteMessage(msgId)
        loadData()
        toast.showSuccess("消息已删除")
    }

    function getFilteredMessages() {
        var filtered = []
        for (var i = 0; i < messageModel.count; i++) {
            var msg = messageModel.get(i)
            var typeMatch = selectedTab === 0 || 
                           (selectedTab === 1 && msg.type === "system") ||
                           (selectedTab === 2 && msg.type === "order") ||
                           (selectedTab === 3 && msg.type === "promotion")
            var searchMatch = searchText === "" || 
                             msg.title.indexOf(searchText) >= 0 || 
                             msg.content.indexOf(searchText) >= 0
            if (typeMatch && searchMatch) {
                filtered.push(msg)
            }
        }
        return filtered
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "9001" && resp.status === "ok") {
                messageModel.clear()
                var list = resp.data.list || []
                unreadCount = 0
                for (var i = 0; i < list.length; ++i) {
                    messageModel.append(list[i])
                    if (list[i].is_read === 0 || list[i].is_read === false) {
                        unreadCount++
                    }
                }
                if (messageModel.count === 0) {
                    // 使用模拟数据
                    messageModel.append({
                        id: 1,
                        title: "🎉 充电成功，赠送优惠券",
                        content: "恭喜您完成一次充电，获得满30减5元优惠券一张，有效期7天。",
                        type: "promotion",
                        is_read: false,
                        create_time: "刚刚"
                    })
                    messageModel.append({
                        id: 2,
                        title: "充电完成通知",
                        content: "您在上海虹桥充电站的充电已完成，共充电45.6度，耗时2小时15分。",
                        type: "order",
                        is_read: false,
                        create_time: "10分钟前"
                    })
                    messageModel.append({
                        id: 3,
                        title: "系统升级通知",
                        content: "ChargePulse将于今晚23:00-次日06:00进行系统维护，届时部分功能将暂停使用。",
                        type: "system",
                        is_read: true,
                        create_time: "1小时前"
                    })
                    unreadCount = 2
                }
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            // 使用模拟数据
            if (messageModel.count === 0) {
                messageModel.append({
                    id: 1,
                    title: "🎉 充电成功，赠送优惠券",
                    content: "恭喜您完成一次充电，获得满30减5元优惠券一张，有效期7天。",
                    type: "promotion",
                    is_read: false,
                    create_time: "刚刚"
                })
                messageModel.append({
                    id: 2,
                    title: "充电完成通知",
                    content: "您在上海虹桥充电站的充电已完成，共充电45.6度，耗时2小时15分。",
                    type: "order",
                    is_read: false,
                    create_time: "10分钟前"
                })
                messageModel.append({
                    id: 3,
                    title: "系统升级通知",
                    content: "ChargePulse将于今晚23:00-次日06:00进行系统维护，届时部分功能将暂停使用。",
                    type: "system",
                    is_read: true,
                    create_time: "1小时前"
                })
                unreadCount = 2
            }
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // ========== 顶部标题栏 ==========
        Rectangle {
            width: parent.width
            height: showSearch ? 110 : 56
            color: "#4A90D9"
            Behavior on height { NumberAnimation { duration: 200 } }
            
            Column {
                anchors.fill: parent
                spacing: 0
                
                Row {
                    width: parent.width
                    height: 56
                    anchors.margins: 8
                    spacing: 8

                    // 返回按钮
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

                    // 标题
                    Text {
                        text: "消息中心"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !showSearch
                    }
                    
                    // 搜索框
                    Rectangle {
                        visible: showSearch
                        width: parent.width - 100
                        height: 36
                        radius: 18
                        color: "#FFFFFF33"
                        anchors.verticalCenter: parent.verticalCenter
                        
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            spacing: 8
                            
                            Text {
                                text: "🔍"
                                font.pixelSize: 16
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                id: txtSearch
                                placeholderText: "搜索消息..."
                                font.pixelSize: 14
                                width: parent.width - 50
                                height: parent.height
                                background: Rectangle { color: "transparent" }
                                verticalAlignment: Text.AlignVCenter
                                onTextChanged: searchText = text
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; visible: !showSearch }

                    // 搜索按钮
                    Text {
                        text: showSearch ? "取消" : "🔍"
                        color: "white"
                        font.pixelSize: 18
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            showSearch = !showSearch
                            if (!showSearch) {
                                searchText = ""
                                txtSearch.text = ""
                            }
                        }
                    }
                    
                    // 未读数量
                    Rectangle {
                        visible: unreadCount > 0 && !showSearch
                        width: 24
                        height: 24
                        radius: 12
                        color: "#E53935"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 4

                        Text {
                            text: unreadCount > 99 ? "99+" : unreadCount
                            color: "white"
                            font.pixelSize: 11
                            font.bold: true
                            anchors.centerIn: parent
                        }
                    }
                    
                    // 全部已读
                    Text {
                        text: unreadCount > 0 ? "全部已读" : ""
                        color: "#BBDEFB"
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                        visible: unreadCount > 0 && !showSearch
                    }
                    MouseArea {
                        anchors.fill: parent
                        visible: unreadCount > 0 && !showSearch
                        onClicked: markAllAsRead()
                    }
                }
                
                // 标签筛选
                Row {
                    visible: !showSearch
                    width: parent.width
                    height: 50
                    spacing: 8
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    
                    Repeater {
                        model: [
                            { text: "全部", count: messageModel.count },
                            { text: "系统", count: 0 },
                            { text: "订单", count: 0 },
                            { text: "活动", count: 0 }
                        ]
                        
                        Rectangle {
                            height: 32
                            radius: 16
                            color: selectedTab === index ? "white" : "#FFFFFF33"
                            
                            Text {
                                text: modelData.text + (modelData.count > 0 ? " (" + modelData.count + ")" : "")
                                color: selectedTab === index ? "#4A90D9" : "white"
                                font.pixelSize: 13
                                font.bold: selectedTab === index
                                anchors.centerIn: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onClicked: selectedTab = index
                            }
                        }
                    }
                }
            }
        }

        // ========== 内容区域 ==========
        Frame {
            width: parent.width
            height: parent.height - (showSearch ? 110 : 106)
            padding: 0
            background: Rectangle { color: "#F5F7FA" }

            // 加载状态
            LoadingOverlay {
                visible: isLoading
                loading: isLoading
                loadingText: "加载消息中..."
                anchors.fill: parent
            }

            // 空状态
            EmptyState {
                visible: messageModel.count === 0 && !isLoading
                emptyIcon: "📭"
                emptyTitle: "暂无消息"
                emptyDescription: searchText ? "未找到相关消息" : "您还没有收到任何消息"
                emptyActionText: searchText ? "清除搜索" : "刷新"
                emptyAction: function() {
                    if (searchText) {
                        searchText = ""
                        txtSearch.text = ""
                    } else {
                        refresh()
                    }
                }
                anchors.fill: parent
            }

            // 消息列表
            ListView {
                visible: messageModel.count > 0
                width: parent.width
                height: parent.height
                model: messageModel
                clip: true
                spacing: 0

                delegate: Rectangle {
                    width: parent.width
                    height: msgContent.height + 24
                    color: model.is_read === 0 || model.is_read === false ? "#FFFFFF" : "#FAFAFA"

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#F0F0F0"
                        anchors.bottom: parent.bottom
                    }
                    
                    // 侧边状态条
                    Rectangle {
                        width: 4
                        height: parent.height - 20
                        color: model.is_read === 0 || model.is_read === false ? "#4A90D9" : "transparent"
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: 10
                        radius: 2
                    }

                    Column {
                        id: msgContent
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 16
                        anchors.topMargin: 12
                        anchors.bottomMargin: 12
                        spacing: 6

                        // 标题行
                        Row {
                            width: parent.width
                            spacing: 8
                            
                            // 类型图标
                            Rectangle {
                                width: 28
                                height: 28
                                radius: 14
                                color: model.type === "system" ? "#E3F2FD" :
                                       model.type === "order" ? "#E8F5E9" :
                                       model.type === "promotion" ? "#FFF3E0" : "#F3E5F5"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Text {
                                    text: model.type === "system" ? "📢" :
                                          model.type === "order" ? "📋" :
                                          model.type === "promotion" ? "🎁" : "📌"
                                    font.pixelSize: 14
                                    anchors.centerIn: parent
                                }
                            }

                            Column {
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Text {
                                    text: model.title || "系统通知"
                                    font.bold: model.is_read === 0 || model.is_read === false
                                    font.pixelSize: 15
                                    color: "#333"
                                    elide: Text.ElideRight
                                    maximumWidth: parent.width - 80
                                }
                                
                                Text {
                                    text: model.create_time || ""
                                    font.pixelSize: 11
                                    color: "#999"
                                }
                            }
                        }

                        // 内容
                        Text {
                            width: parent.width
                            text: model.content || ""
                            font.pixelSize: 13
                            color: "#666"
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }

                        // 标签和操作
                        Row {
                            spacing: 8
                            
                            // 类型标签
                            Rectangle {
                                height: 20
                                radius: 10
                                color: model.type === "system" ? "#E3F2FD" :
                                       model.type === "order" ? "#E8F5E9" :
                                       model.type === "promotion" ? "#FFF3E0" : "#F3E5F5"

                                Text {
                                    text: model.type === "system" ? "系统通知" :
                                          model.type === "order" ? "订单消息" :
                                          model.type === "promotion" ? "活动优惠" : "其他"
                                    font.pixelSize: 10
                                    color: model.type === "system" ? "#1976D2" :
                                           model.type === "order" ? "#388E3C" :
                                           model.type === "promotion" ? "#F57C00" : "#7B1FA2"
                                    anchors.centerIn: parent
                                    anchors.margins: 8
                                }
                            }
                            
                            // 新消息标签
                            Rectangle {
                                visible: model.is_read === 0 || model.is_read === false
                                height: 20
                                radius: 10
                                color: "#E53935"

                                Text {
                                    text: " NEW "
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: "white"
                                    anchors.centerIn: parent
                                }
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            // 删除按钮
                            Text {
                                text: "🗑"
                                font.pixelSize: 16
                                color: "#999"
                                visible: swipeArea.progress !== 0 || deleteHovered
                                
                                property bool deleteHovered: false
                                
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: parent.deleteHovered = true
                                    onExited: parent.deleteHovered = false
                                    onClicked: {
                                        deleteMessage(model.id)
                                    }
                                }
                            }
                        }
                    }
                    
                    // 滑动删除
                    SwipeDelegate {
                        id: swipeArea
                        anchors.fill: parent
                        visible: false  // 隐藏默认滑动
                    }
                    
                    // 点击区域
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (model.is_read === 0 || model.is_read === false) {
                                markAsRead(model.id)
                            }
                            root.stackView.push(messageDetailPageComponent, { "messageId": model.id })
                        }
                    }
                }

                footer: Item { height: 20 }
            }
        }
    }

    header: null
}
