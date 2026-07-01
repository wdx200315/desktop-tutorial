import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "设置"

    Connections {
        target: net
        function onResponseReceived(resp) {
            if (resp.cmd === "1003" && resp.status === "ok") {
                toast.show("密码修改成功")
                dlgChangePassword.close()
                txtOldPwd.text = ""
                txtNewPwd.text = ""
                txtConfirmPwd.text = ""
            }
            if (resp.status === "error") {
                toast.show("错误: " + resp.message)
            }
        }
    }

    // 密码修改对话框
    Dialog {
        id: dlgChangePassword
        title: "修改密码"
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        width: parent.width * 0.9

        Column {
            spacing: 12
            anchors.margins: 16
            width: parent.width - 32

            TextField {
                id: txtOldPwd
                width: parent.width
                placeholderText: "请输入原密码"
                echoMode: TextInput.Password
            }

            TextField {
                id: txtNewPwd
                width: parent.width
                placeholderText: "请输入新密码 (至少8位)"
                echoMode: TextInput.Password
            }

            TextField {
                id: txtConfirmPwd
                width: parent.width
                placeholderText: "请再次输入新密码"
                echoMode: TextInput.Password
            }

            Text {
                text: "密码要求：至少8位，包含字母和数字"
                color: "#757575"
                font.pixelSize: 12
            }
        }

        onAccepted: {
            if (txtOldPwd.text.length === 0) {
                toast.show("请输入原密码")
                return
            }
            if (txtNewPwd.text.length < 8) {
                toast.show("新密码至少8位")
                return
            }
            if (txtNewPwd.text !== txtConfirmPwd.text) {
                toast.show("两次密码不一致")
                return
            }
            if (!net.connected) {
                toast.show("未连接服务端")
                return
            }
            net.changePassword(txtOldPwd.text, txtNewPwd.text)
        }
    }

    // 支付方式设置对话框
    Dialog {
        id: dlgPaymentMethod
        title: "支付方式"
        standardButtons: Dialog.Ok
        modal: true
        width: parent.width * 0.9

        Column {
            spacing: 12
            anchors.margins: 16
            width: parent.width - 32

            CheckBox {
                id: chkBalance
                text: "账户余额"
                checked: true
                enabled: false
            }

            CheckBox {
                id: chkWechat
                text: "微信支付"
            }

            CheckBox {
                id: chkAlipay
                text: "支付宝"
            }

            CheckBox {
                id: chkCoupon
                text: "优惠券"
            }
        }
    }

    // 主题选择对话框
    Dialog {
        id: dlgTheme
        title: "主题设置"
        standardButtons: Dialog.Ok
        modal: true
        width: parent.width * 0.9

        Column {
            spacing: 12
            anchors.margins: 16
            width: parent.width - 32

            RadioButton {
                id: radioLight
                text: "浅色主题"
                checked: true
            }

            RadioButton {
                id: radioDark
                text: "深色主题"
            }

            RadioButton {
                id: radioAuto
                text: "跟随系统"
            }
        }
    }

    // 关于对话框
    Dialog {
        id: dlgAbout
        title: "关于 ChargePulse"
        standardButtons: Dialog.Ok
        modal: true
        width: parent.width * 0.9

        Column {
            spacing: 12
            anchors.margins: 16
            width: parent.width - 32
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                text: "ChargePulse One"
                font.bold: true
                font.pixelSize: 18
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "版本: v1.0.0"
                color: "#757575"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "全能型充电运营管理平台"
                color: "#757575"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#E0E0E0"
            }

            Text {
                text: "© 2026 ChargePulse"
                color: "#9E9E9E"
                font.pixelSize: 12
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Column {
        spacing: 0
        anchors.fill: parent

        // 账号安全
        SectionHeader {
            text: "账号安全"
        }

        Button {
            text: "修改密码"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "🔑"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "修改密码"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                dlgChangePassword.open()
            }
        }

        Button {
            text: "绑定手机"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "📱"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "绑定手机"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                stack.push("ProfilePage.qml")
            }
        }

        // 支付设置
        SectionHeader {
            text: "支付设置"
        }

        Button {
            text: "支付方式"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "💳"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "支付方式"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                dlgPaymentMethod.open()
            }
        }

        Button {
            text: "充值记录"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "📜"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "充值记录"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                stack.push("OrderPage.qml")
            }
        }

        // 通用设置
        SectionHeader {
            text: "通用设置"
        }

        Button {
            text: "主题设置"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "🎨"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "主题设置"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                dlgTheme.open()
            }
        }

        Button {
            text: "清除缓存"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "🗑️"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "清除缓存"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                toast.show("缓存已清除")
            }
        }

        // 其他
        SectionHeader {
            text: "其他"
        }

        Button {
            text: "关于我们"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "ℹ️"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "关于我们"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                dlgAbout.open()
            }
        }

        Button {
            text: "联系客服"
            flat: true
            width: parent.width
            height: 52
            contentItem: Row {
                spacing: 12
                anchors.left: parent.left
                anchors.leftMargin: 16
                Text { text: "📞"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "联系客服"; font.pixelSize: 16; anchors.verticalCenter: parent.verticalCenter }
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#E0E0E0"
            }
            onClicked: {
                toast.show("客服热线: 400-xxx-xxxx")
            }
        }

        // 退出登录
        Rectangle {
            width: parent.width
            height: 8
            color: "#F5F5F5"
        }

        Button {
            text: "退出登录"
            flat: true
            width: parent.width
            height: 52
            contentItem: Text {
                text: "退出登录"
                font.pixelSize: 16
                color: "#EF5350"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            onClicked: {
                net.logout()
                toast.show("已退出登录")
                stack.pop(null)
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
                text: "设置"
                font.bold: true
                Layout.fillWidth: true
            }
        }
    }
}
