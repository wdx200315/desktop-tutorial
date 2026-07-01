import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "告警列表"
    property var alarmModel: ListModel { }
    property bool isLoading: false
    property int currentTabIndex: 0

    // 标签页数据
    property var tabLabels: ["全部", "活跃", "已处理"]

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            isLoading = true
            net.getAlarmList(1, 50)
        } else {
            toast.showWarning("网络未连接")
        }
    }

    function refresh() {
        loadData()
    }

    function handleAlarm(alarmId) {
        toast.showSuccess("告警已处理")
        loadData()
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "6001" && resp.status === "ok") {
                alarmModel.clear()
                var list = resp.data || []
                for (var i = 0; i < list.length; ++i) {
                    alarmModel.append(list[i])
                }
                if (alarmModel.count === 0) {
                    toast.showInfo("暂无告警数据")
                }
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
                    text: "告警列表"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "🔄"
                    color: "white"
                    font.pixelSize: 18
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: refresh()
                }
            }
        }

        // ========== 标签页 ==========
        Rectangle {
            width: parent.width
            height: 44
            color: "white"

            Row {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Repeater {
                    model: tabLabels.length

                    Rectangle {
                        width: parent.width / tabLabels.length
                        height: parent.height
                        color: "transparent"

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Text {
                                text: tabLabels[index]
                                font.pixelSize: 14
                                color: currentTabIndex === index ? "#4A90D9" : "#666666"
                                font.bold: currentTabIndex === index
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            Rectangle {
                                width: 24
                                height: 3
                                radius: 1.5
                                color: currentTabIndex === index ? "#4A90D9" : "transparent"
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentTabIndex = index
                        }
                    }
                }
            }
        }

        // 分隔线
        Rectangle {
            width: parent.width
            height: 1
            color: "#F0F0F0"
        }

        // ========== 内容区域 ==========
        Frame {
            width: parent.width
            height: parent.height - 100
            padding: 0
            background: Rectangle { color: "#F5F7FA" }

            // 加载状态
            LoadingOverlay {
                visible: isLoading
                loading: isLoading
                loadingText: "加载告警中..."
                anchors.fill: parent
            }

            // 空状态
            EmptyState {
                visible: alarmModel.count === 0 && !isLoading
                emptyIcon: "✅"
                emptyTitle: "暂无告警"
                emptyDescription: currentTabIndex === 0 ? "系统运行正常，无告警" :
                                 currentTabIndex === 1 ? "暂无活跃告警" : "暂无已处理告警"
                emptyActionText: "刷新"
                emptyAction: function() {
                    refresh()
                }
                anchors.fill: parent
            }

            // 告警列表
            ListView {
                visible: alarmModel.count > 0
                width: parent.width
                height: parent.height
                model: alarmModel
                clip: true
                spacing: 0

                delegate: Rectangle {
                    width: parent.width
                    height: 100
                    color: "white"

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#F0F0F0"
                        anchors.bottom: parent.bottom
                    }

                    // 告警级别标签
                    Rectangle {
                        width: 50
                        height: 22
                        radius: 11
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.topMargin: 12
                        anchors.leftMargin: 16
                        color: model.level === "critical" ? "#FFEBEE" :
                               model.level === "warning" ? "#FFF3E0" : "#E3F2FD"

                        Text {
                            text: model.level === "critical" ? "严重" :
                                  model.level === "warning" ? "警告" : "提示"
                            font.pixelSize: 11
                            color: model.level === "critical" ? "#E53935" :
                                   model.level === "warning" ? "#FF9800" : "#1976D2"
                            anchors.centerIn: parent
                        }
                    }

                    // 状态标签
                    Rectangle {
                        width: 60
                        height: 22
                        radius: 11
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 12
                        anchors.rightMargin: 16
                        color: model.status === "active" ? "#FFEBEE" : "#E8F5E9"

                        Text {
                            text: model.status === "active" ? "活跃" : "已处理"
                            font.pixelSize: 11
                            color: model.status === "active" ? "#E53935" : "#4CAF50"
                            anchors.centerIn: parent
                        }
                    }

                    Column {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 40
                        anchors.bottomMargin: 12
                        spacing: 6

                        // 告警标题
                        Text {
                            text: model.title || ("充电桩 #" + (model.charger_id || "未知") + " 告警")
                            font.bold: true
                            font.pixelSize: 14
                            color: "#333"
                            elide: Text.ElideRight
                        }

                        // 告警内容
                        Text {
                            text: model.message || "检测到异常"
                            font.pixelSize: 12
                            color: "#666"
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }

                        // 时间和操作
                        Row {
                            width: parent.width

                            Text {
                                text: model.create_time || ""
                                font.pixelSize: 11
                                color: "#999"
                            }

                            Item { Layout.fillWidth: true }

                            // 处理按钮
                            Rectangle {
                                visible: model.status === "active"
                                width: 70
                                height: 26
                                radius: 13
                                color: "#4A90D9"

                                Text {
                                    text: "处理"
                                    color: "white"
                                    font.pixelSize: 12
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        handleAlarm(model.id)
                                    }
                                }
                            }
                        }
                    }
                }

                footer: Item { height: 20 }
            }
        }

        // 底部间距
        Item { height: 10 }
    }

    header: null
}
