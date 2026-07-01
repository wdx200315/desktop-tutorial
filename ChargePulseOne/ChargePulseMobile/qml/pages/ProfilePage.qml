import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "个人信息"
    property var userData: ({
        username: "",
        phone: "",
        email: "",
        plate_number: "",
        member_level: "",
        balance: 0,
        created_at: ""
    })

    Component.onCompleted: {
        refresh()
    }

    function refresh() {
        if (!net.connected) {
            toast.show("未连接服务端")
            return
        }
        net.getUserInfo()
    }

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "1005" && resp.status === "ok") {
                userData = resp.data
                txtUsername.text = userData.username || ""
                txtPhone.text = userData.phone || ""
                txtEmail.text = userData.email || ""
                txtPlate.text = userData.plate_number || ""
                lblMember.text = "会员等级: " + (userData.member_level || "普通会员")
                lblBalance.text = "余额: ¥" + (parseFloat(userData.balance) || 0).toFixed(2)
                lblCreated.text = "注册时间: " + (userData.created_at || "未知")
            }
            if (resp.cmd === "1006" && resp.status === "ok") {
                toast.show("个人信息更新成功")
                refresh()
            }
            if (resp.status === "error") {
                toast.show("错误: " + resp.message)
            }
        }
        function onErrorOccurred(msg) {
            toast.show("网络错误: " + msg)
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 用户头像区域
        Rectangle {
            width: parent.width
            height: 80
            color: "#1A73E8"
            radius: 12
            Row {
                anchors.centerIn: parent
                spacing: 16
                Rectangle {
                    width: 60
                    height: 60
                    radius: 30
                    color: "#ffffff"
                    Text {
                        anchors.centerIn: parent
                        text: (txtUsername.text || "U")[0].toUpperCase()
                        font.pixelSize: 28
                        font.bold: true
                        color: "#1A73E8"
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        id: lblMember
                        color: "#ffffff"
                        font.pixelSize: 14
                    }
                    Text {
                        id: lblBalance
                        color: "#ffffff"
                        font.pixelSize: 18
                        font.bold: true
                    }
                }
            }
        }

        // 注册时间
        Text {
            id: lblCreated
            color: "#757575"
            font.pixelSize: 12
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // 表单区域
        Rectangle {
            width: parent.width
            height: formColumn.height + 32
            color: "#FAFAFA"
            radius: 8

            Column {
                id: formColumn
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: "基本信息"
                    font.bold: true
                    font.pixelSize: 16
                }

                Row {
                    spacing: 12
                    width: parent.width
                    Text {
                        text: "用户名"
                        width: 70
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextField {
                        id: txtUsername
                        width: parent.width - 70
                        placeholderText: "用户名"
                        enabled: false  // 用户名不可修改
                        background: Rectangle {
                            color: "#E0E0E0"
                            radius: 4
                        }
                    }
                }

                Row {
                    spacing: 12
                    width: parent.width
                    Text {
                        text: "手机号"
                        width: 70
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextField {
                        id: txtPhone
                        width: parent.width - 70
                        placeholderText: "请输入手机号"
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                }

                Row {
                    spacing: 12
                    width: parent.width
                    Text {
                        text: "邮箱"
                        width: 70
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextField {
                        id: txtEmail
                        width: parent.width - 70
                        placeholderText: "请输入邮箱"
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                    }
                }

                Row {
                    spacing: 12
                    width: parent.width
                    Text {
                        text: "车牌号"
                        width: 70
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    TextField {
                        id: txtPlate
                        width: parent.width - 70
                        placeholderText: "请输入车牌号"
                    }
                }
            }
        }

        // 保存按钮
        Button {
            text: "保存修改"
            width: parent.width
            height: 48
            onClicked: {
                if (!net.connected) {
                    toast.show("未连接服务端")
                    return
                }
                var data = {
                    phone: txtPhone.text,
                    email: txtEmail.text,
                    plate_number: txtPlate.text
                }
                net.updateUserInfo(data)
            }
        }

        Item { Layout.fillHeight: true }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "←"
                onClicked: stack.pop()
            }
            Label {
                text: "个人信息"
                font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                text: "刷新"
                onClicked: refresh()
            }
        }
    }
}
