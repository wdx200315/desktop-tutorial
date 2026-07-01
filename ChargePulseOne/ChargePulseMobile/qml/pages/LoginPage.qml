import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"

Page {
    title: "登录"
    property string username: ""
    property string password: ""
    property bool isLoading: false
    property bool rememberMe: true  // 记住登录状态
    
    // 背景渐变
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#4A90D9" }
            GradientStop { position: 1.0; color: "#1A5F9E" }
        }
    }
    
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
    
    Rectangle {
        width: 200
        height: 200
        radius: 100
        color: "#FFFFFF"
        opacity: 0.05
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -50
        anchors.leftMargin: -50
    }
    
    Flickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + 100
        
        Column {
            id: contentColumn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 80
            spacing: 30
            width: parent.width - 60
            
            // Logo区域
            Column {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter
                
                // Logo图标
                Rectangle {
                    width: 100
                    height: 100
                    radius: 25
                    color: "#FFFFFF"
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    // 内阴影效果
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.width: 2
                        border.color: "#FFFFFF33"
                    }
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        
                        Text {
                            text: "⚡"
                            font.pixelSize: 40
                            color: "#4A90D9"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "Charge"
                            color: "#4A90D9"
                            font.bold: true
                            font.pixelSize: 12
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                
                Text {
                    text: "ChargePulse"
                    font.bold: true
                    font.pixelSize: 28
                    color: "#FFFFFF"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "让充电更简单"
                    font.pixelSize: 16
                    color: "#FFFFFF"
                    opacity: 0.9
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            
            // 登录表单卡片
            Rectangle {
                width: parent.width
                radius: 16
                color: "#FFFFFF"
                anchors.horizontalCenter: parent.horizontalCenter
                
                // 卡片阴影
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: parent.color
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    opacity: 0.3
                }
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 20
                    
                    Text {
                        text: "欢迎回来"
                        font.bold: true
                        font.pixelSize: 20
                        color: "#333333"
                    }
                    
                    Text {
                        text: "请登录您的账号"
                        font.pixelSize: 14
                        color: "#666666"
                    }
                    
                    // 用户名输入
                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 12
                        color: "#F5F7FA"
                        border.width: 1
                        border.color: txtUsername.focus ? "#4A90D9" : "#E0E0E0"
                        
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12
                            
                            Text {
                                text: "👤"
                                font.pixelSize: 20
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                id: txtUsername
                                placeholderText: "请输入用户名/手机号"
                                font.pixelSize: 15
                                width: parent.width - 50
                                height: parent.height
                                background: Rectangle { color: "transparent" }
                                verticalAlignment: Text.AlignVCenter
                                onFocusChanged: {
                                    if (focus) parent.parent.border.color = "#4A90D9"
                                }
                            }
                        }
                    }
                    
                    // 密码输入
                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 12
                        color: "#F5F7FA"
                        border.width: 1
                        border.color: txtPassword.focus ? "#4A90D9" : "#E0E0E0"
                        
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12
                            
                            Text {
                                text: "🔒"
                                font.pixelSize: 20
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                id: txtPassword
                                placeholderText: "请输入密码"
                                font.pixelSize: 15
                                width: parent.width - 80
                                height: parent.height
                                echoMode: TextInput.Password
                                background: Rectangle { color: "transparent" }
                                verticalAlignment: Text.AlignVCenter
                                onAccepted: loginBtn.clicked()
                            }
                            
                            // 显示/隐藏密码
                            Text {
                                text: txtPassword.echoMode === TextInput.Password ? "👁" : "👁‍🗨"
                                font.pixelSize: 18
                                anchors.verticalCenter: parent.verticalCenter
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        txtPassword.echoMode = txtPassword.echoMode === TextInput.Password ? TextInput.Normal : TextInput.Password
                                    }
                                }
                            }
                        }
                    }
                    
                    // 记住我和忘记密码
                    Row {
                        width: parent.width
                        
                        // 记住我
                        Row {
                            spacing: 8
                            
                            Rectangle {
                                width: 22
                                height: 22
                                radius: 6
                                color: rememberMe ? "#4A90D9" : "transparent"
                                border.width: 2
                                border.color: rememberMe ? "#4A90D9" : "#CCCCCC"
                                
                                Text {
                                    text: "✓"
                                    font.pixelSize: 14
                                    color: "white"
                                    anchors.centerIn: parent
                                    visible: rememberMe
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: rememberMe = !rememberMe
                                }
                            }
                            
                            Text {
                                text: "记住我"
                                font.pixelSize: 14
                                color: "#666666"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Text {
                            text: "忘记密码？"
                            font.pixelSize: 14
                            color: "#4A90D9"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                toast.showInfo("密码找回功能开发中")
                            }
                        }
                    }
                    
                    // 登录按钮
                    Rectangle {
                        id: loginBtn
                        width: parent.width
                        height: 52
                        radius: 26
                        color: isLoading ? "#CCCCCC" : "#4A90D9"
                        
                        Text {
                            text: isLoading ? "登录中..." : "登 录"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 17
                            anchors.centerIn: parent
                        }
                        
                        // 加载动画
                        Rectangle {
                            visible: isLoading
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            radius: 12
                            color: "transparent"
                            border.width: 3
                            border.color: "#FFFFFF"
                            
                            RotationAnimation on rotation {
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 800
                                running: isLoading
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: performLogin()
                        }
                    }
                }
            }
            
            // 其他登录方式
            Column {
                width: parent.width
                spacing: 16
                anchors.horizontalCenter: parent.horizontalCenter
                
                Text {
                    text: "其他登录方式"
                    font.pixelSize: 13
                    color: "#FFFFFF"
                    opacity: 0.8
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Row {
                    spacing: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    // 微信登录
                    Rectangle {
                        width: 56
                        height: 56
                        radius: 28
                        color: "#FFFFFF"
                        
                        Text {
                            text: "💬"
                            font.pixelSize: 28
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: toast.showInfo("微信登录开发中")
                        }
                    }
                    
                    // 手机号登录
                    Rectangle {
                        width: 56
                        height: 56
                        radius: 28
                        color: "#FFFFFF"
                        
                        Text {
                            text: "📱"
                            font.pixelSize: 28
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: toast.showInfo("手机号登录开发中")
                        }
                    }
                    
                    // 支付宝登录
                    Rectangle {
                        width: 56
                        height: 56
                        radius: 28
                        color: "#FFFFFF"
                        
                        Text {
                            text: "💙"
                            font.pixelSize: 28
                            anchors.centerIn: parent
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: toast.showInfo("支付宝登录开发中")
                        }
                    }
                }
            }
            
            // 注册入口
            Row {
                spacing: 4
                anchors.horizontalCenter: parent.horizontalCenter
                
                Text {
                    text: "还没有账号？"
                    font.pixelSize: 14
                    color: "#FFFFFF"
                }
                
                Text {
                    text: "立即注册"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#FFD700"
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        toast.showInfo("注册功能开发中")
                    }
                }
            }
        }
    }
    
    // 底部协议
    Row {
        spacing: 4
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        
        Text {
            text: "登录即表示同意"
            font.pixelSize: 11
            color: "#FFFFFF"
            opacity: 0.7
        }
        
        Text {
            text: "《用户协议》"
            font.pixelSize: 11
            color: "#FFD700"
        }
        
        Text {
            text: "和"
            font.pixelSize: 11
            color: "#FFFFFF"
            opacity: 0.7
        }
        
        Text {
            text: "《隐私政策》"
            font.pixelSize: 11
            color: "#FFD700"
        }
    }
    
    // 登录函数
    function performLogin() {
        var user = txtUsername.text.trim()
        var pwd = txtPassword.text
        
        // 验证
        if (user === "") {
            toast.showWarning("请输入用户名或手机号")
            return
        }
        
        if (pwd === "") {
            toast.showWarning("请输入密码")
            return
        }
        
        if (pwd.length < 6) {
            toast.showWarning("密码长度不能少于6位")
            return
        }
        
        isLoading = true
        net.login(user, pwd)
    }
    
    Connections {
        target: net
        function onResponseReceived(resp) {
            isLoading = false
            if (resp.cmd === "1001" && resp.status === "ok") {
                toast.showSuccess("登录成功，欢迎回来！")
                
                // 保存登录状态
                session.token = resp.token || net.token
                session.username = txtUsername.text
                session.isLoggedIn = true
                
                // 跳转到首页
                root.stackView.replace(homePageComponent)
            } else if (resp.cmd === "1001" && resp.status === "error") {
                toast.showError(resp.message || "用户名或密码错误")
            }
        }
        function onErrorOccurred(msg) {
            isLoading = false
            toast.showError("网络连接失败: " + msg)
        }
    }
    
    header: null
}
