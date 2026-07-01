import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Dialogs 6.0
import "../components"

Page {
    title: "我的车辆"

    property var model: ListModel { }

    Component.onCompleted: refresh()

    function refresh() {
        // 模拟数据
        model.clear()
        model.append({id: 1, plate_number: "浙A12345", brand: "特斯拉", model: "Model 3"})
        model.append({id: 2, plate_number: "浙B67890", brand: "比亚迪", model: "汉EV"})
    }

    Column {
        anchors.fill: parent

        // 车辆列表
        ListView {
            width: parent.width
            height: parent.height - 80
            model: model
            clip: true

            header: Rectangle {
                width: parent.width
                height: 50
                color: "#f5f5f5"
                Text {
                    anchors.centerIn: parent
                    text: "已绑定 " + model.count + " 辆车"
                    color: "#666"
                }
            }

            delegate: Rectangle {
                width: parent.width
                height: 70
                color: "white"

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#f0f0f0"
                    anchors.bottom: parent.bottom
                }

                Row {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Rectangle {
                        width: 50
                        height: 50
                        radius: 25
                        color: "#4A90D9"

                        Text {
                            anchors.centerIn: parent
                            text: model.plate_number.charAt(model.plate_number.length - 2) + model.plate_number.charAt(model.plate_number.length - 1)
                            color: "white"
                            font.bold: true
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: model.plate_number
                            font.bold: true
                            font.pixelSize: 16
                        }

                        Text {
                            text: model.brand + " " + model.model
                            color: "#666"
                            font.pixelSize: 13
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "删除"
                        flat: true
                        onClicked: {
                            var req = {
                                "cmd": "1011",
                                "data": { "id": model.id },
                                "token": session.token
                            }
                            net.sendRequest(req)
                            refresh()
                        }
                    }
                }
            }

            footer: Item { height: 20 }
        }

        // 添加按钮
        Rectangle {
            width: parent.width
            height: 60
            color: "#4A90D9"

            Text {
                anchors.centerIn: parent
                text: "➕ 添加车辆"
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: addDialog.open()
            }
        }
    }

    // 添加车辆对话框
    Dialog {
        id: addDialog
        title: "添加车辆"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true

        Column {
            spacing: 12
            width: parent.width

            Text {
                text: "车牌号"
                font.bold: true
            }
            TextField {
                id: plateInput
                placeholderText: "请输入车牌号，如：浙A12345"
                width: parent.width
            }

            Text {
                text: "品牌"
                font.bold: true
            }
            TextField {
                id: brandInput
                placeholderText: "请输入品牌，如：特斯拉"
                width: parent.width
            }

            Text {
                text: "车型"
                font.bold: true
            }
            TextField {
                id: modelInput
                placeholderText: "请输入车型，如：Model 3"
                width: parent.width
            }
        }

        onAccepted: {
            var plate = plateInput.text.trim()
            var brand = brandInput.text.trim()
            var modelName = modelInput.text.trim()

            if (plate && brand && modelName) {
                var req = {
                    "cmd": "1009",
                    "data": { "plate_number": plate, "brand": brand, "model": modelName },
                    "token": session.token
                }
                net.sendRequest(req)
                refresh()

                // 清空输入
                plateInput.text = ""
                brandInput.text = ""
                modelInput.text = ""
            }
        }
    }
}
