import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "订单详情"
    property int orderId: 0
    property var orderData: ({})
    property var curveData: []

    Component.onCompleted: {
        if (orderId > 0) {
            var req = { "cmd": "5002", "data": { "id": orderId }, "token": session.token }
            net.sendRequest(req)
        }
        // 模拟历史功率数据（实际可从服务端获取）
        curveData = []
        for (var i = 0; i < 30; ++i) {
            curveData.push({ time: i, power: Math.random() * 60 + 10 })
        }
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "5002" && resp.status === "ok") {
                orderData = resp.data
                lblOrderId.text = "订单 #" + orderData.id
                lblStatus.text = "状态: " + orderData.status
                lblMode.text = "模式: " + orderData.mode
                lblStart.text = "开始: " + orderData.start_time
                lblEnd.text = "结束: " + (orderData.end_time || "进行中")
                lblEnergy.text = "电量: " + (orderData.energy_kwh || "0") + " kWh"
                lblAmount.text = "金额: ¥" + (orderData.amount || "0.00")
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Grid {
            columns: 2
            spacing: 8
            width: parent.width
            Text { text: "订单号:"; font.bold: true }
            Text { id: lblOrderId; text: "加载中..." }
            Text { text: "状态:"; font.bold: true }
            Text { id: lblStatus; text: ""; color: "blue" }
            Text { text: "模式:"; font.bold: true }
            Text { id: lblMode; text: "" }
            Text { text: "开始时间:"; font.bold: true }
            Text { id: lblStart; text: "" }
            Text { text: "结束时间:"; font.bold: true }
            Text { id: lblEnd; text: "" }
            Text { text: "电量:"; font.bold: true }
            Text { id: lblEnergy; text: "" }
            Text { text: "金额:"; font.bold: true }
            Text { id: lblAmount; text: "" }
        }

        Rectangle {
            width: parent.width
            height: 150
            color: "#f5f5f5"
            radius: 8
            Canvas {
                anchors.fill: parent
                anchors.margins: 10
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (curveData.length === 0) return
                    ctx.strokeStyle = "#999"
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(0, height)
                    ctx.lineTo(width, height)
                    ctx.stroke()
                    ctx.strokeStyle = "#1A73E8"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    var max = 0
                    for (var i = 0; i < curveData.length; ++i) {
                        if (curveData[i].power > max) max = curveData[i].power
                    }
                    max = max * 1.2 || 60
                    for (var j = 0; j < curveData.length; ++j) {
                        var x = (j / (curveData.length - 1)) * width
                        var y = height - (curveData[j].power / max) * height
                        if (j === 0) ctx.moveTo(x, y)
                        else ctx.lineTo(x, y)
                    }
                    ctx.stroke()
                }
            }
            Text {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 5
                text: "功率 (kW)"
                font.pixelSize: 10
                color: "#999"
            }
        }

        Button { text: "返回"; onClicked: stack.pop() }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton { text: "←"; onClicked: stack.pop() }
            Label { text: "订单详情"; font.bold: true; Layout.fillWidth: true }
        }
    }
}
