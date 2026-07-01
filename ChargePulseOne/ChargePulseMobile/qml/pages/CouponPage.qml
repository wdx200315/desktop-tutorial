import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "优惠券"
    property var couponModel: ListModel { }
    property bool isLoading: false
    property int currentTabIndex: 0

    // 标签页数据
    property var tabLabels: ["可使用", "已使用", "已过期"]

    Component.onCompleted: {
        loadData()
    }

    function loadData() {
        if (net.connected) {
            isLoading = true
            net.getCouponList()
        } else {
            toast.showWarning("网络未连接")
        }
    }

    function refresh() {
        loadData()
    }

    function claimCoupon(couponId) {
        if (net.connected) {
            net.claimCoupon(couponId)
            toast.showSuccess("优惠券领取成功")
        } else {
            toast.showError("网络未连接")
        }
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "9101" && resp.status === "ok") {
                couponModel.clear()
                var list = resp.data || []
                for (var i = 0; i < list.length; ++i) {
                    couponModel.append(list[i])
                }
                if (couponModel.count === 0) {
                    toast.showInfo("暂无可用优惠券")
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
                    text: "我的优惠券"
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
                loadingText: "加载优惠券中..."
                anchors.fill: parent
            }

            // 空状态
            EmptyState {
                visible: couponModel.count === 0 && !isLoading
                emptyIcon: "🎫"
                emptyTitle: "暂无优惠券"
                emptyDescription: currentTabIndex === 0 ? "您还没有可用的优惠券" :
                                 currentTabIndex === 1 ? "暂无已使用的优惠券" : "暂无过期的优惠券"
                emptyActionText: currentTabIndex === 0 ? "去领取" : ""
                emptyAction: function() {
                    toast.showInfo("更多优惠券，敬请期待")
                }
                anchors.fill: parent
            }

            // 优惠券列表
            ListView {
                visible: couponModel.count > 0
                width: parent.width
                height: parent.height
                model: couponModel
                clip: true
                spacing: 12
                padding: 12

                delegate: Rectangle {
                    width: parent.width - 24
                    height: 120
                    radius: 12
                    color: "white"
                    anchors.horizontalCenter: parent.horizontalCenter

                    // 阴影效果
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: parent.color
                        opacity: 0.3
                        anchors.top: parent.top
                        anchors.topMargin: 3
                    }

                    Row {
                        anchors.fill: parent
                        anchors.margins: 0

                        // 左侧金额区域
                        Rectangle {
                            width: 100
                            height: parent.height
                            radius: 12
                            color: model.type === "discount" ? "#FF9800" :
                                   model.type === "cash" ? "#4CAF50" : "#4A90D9"

                            Column {
                                anchors.centerIn: parent
                                spacing: 4

                                Text {
                                    text: "¥"
                                    color: "white"
                                    font.pixelSize: 14
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: model.value || model.discount || "0"
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 28
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: model.type === "discount" ? "折" :
                                           model.type === "cash" ? "元" : "元"
                                    color: "white"
                                    font.pixelSize: 12
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }

                        // 右侧信息区域
                        Column {
                            width: parent.width - 100
                            height: parent.height
                            anchors.leftMargin: 12
                            anchors.topMargin: 12
                            anchors.bottomMargin: 12
                            spacing: 6

                            // 优惠券名称
                            Text {
                                text: model.name || "优惠券"
                                font.bold: true
                                font.pixelSize: 15
                                color: "#333"
                            }

                            // 使用条件
                            Text {
                                text: "满" + (model.min_amount || 0) + "元可用"
                                font.pixelSize: 12
                                color: "#666"
                            }

                            // 有效期
                            Text {
                                text: "有效期至: " + (model.end_time || "无限期")
                                font.pixelSize: 11
                                color: "#999"
                            }

                            Item { Layout.fillHeight: true }

                            // 状态标签和按钮
                            Row {
                                spacing: 8

                                // 状态标签
                                Rectangle {
                                    width: 60
                                    height: 22
                                    radius: 11
                                    color: model.status === "available" ? "#E8F5E9" :
                                           model.status === "used" ? "#F5F5F5" : "#FFEBEE"

                                    Text {
                                        text: model.status === "available" ? "可使用" :
                                               model.status === "used" ? "已使用" : "已过期"
                                        font.pixelSize: 10
                                        color: model.status === "available" ? "#4CAF50" :
                                               model.status === "used" ? "#999" : "#E53935"
                                        anchors.centerIn: parent
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // 使用按钮
                                Rectangle {
                                    visible: model.status === "available"
                                    width: 70
                                    height: 28
                                    radius: 14
                                    color: "#4A90D9"

                                    Text {
                                        text: "立即使用"
                                        color: "white"
                                        font.pixelSize: 12
                                        font.bold: true
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            root.stackView.push(scanPageComponent)
                                        }
                                    }
                                }

                                // 领取按钮
                                Rectangle {
                                    visible: model.can_claim === true && model.status !== "used"
                                    width: 70
                                    height: 28
                                    radius: 14
                                    color: "#FF9800"

                                    Text {
                                        text: "立即领取"
                                        color: "white"
                                        font.pixelSize: 12
                                        font.bold: true
                                        anchors.centerIn: parent
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            claimCoupon(model.id)
                                        }
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
