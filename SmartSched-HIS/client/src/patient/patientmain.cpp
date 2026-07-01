/**
 * @file patientmain.cpp
 * @brief 患者挂号终端 - 主程序入口
 * 
 * 功能：触摸式挂号界面，选择科室和医生，获取排队号
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFont>
#include <QIcon>

#include "smartsched/client/networkclient.h"
#include "smartsched/client/datamodel.h"
#include "smartsched/common/version.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // 应用信息
    app.setApplicationName("SmartSched-患者挂号");
    app.setApplicationVersion(SMARTSCHED_VERSION_STRING);
    app.setOrganizationName("SmartSched Healthcare");
    
    // 设置默认样式
    QQuickStyle::setStyle("Material");
    
    // 创建网络客户端（单例）
    auto networkClient = new smartsched::client::NetworkClient(&app);
    
    // 创建数据模型
    auto departmentModel = new smartsched::client::DepartmentModel(&app);
    auto queueModel = new smartsched::client::QueueModel(&app);
    auto patientModel = new smartsched::client::PatientModel(&app);
    auto doctorModel = new smartsched::client::DoctorModel(&app);
    
    // 配置网络客户端
    smartsched::client::NetworkClientConfig config;
    config.host = "localhost";
    config.port = 8888;
    config.autoReconnect = true;
    config.heartbeatInterval = 30;
    networkClient->setConfig(config);
    
    // 连接信号
    QObject::connect(networkClient, &smartsched::client::NetworkClient::connected, [&]() {
        qDebug() << "Connected to server";
    });
    
    QObject::connect(networkClient, &smartsched::client::NetworkClient::disconnected, [&]() {
        qDebug() << "Disconnected from server";
    });
    
    // 创建QML引擎
    QQmlApplicationEngine engine;
    
    // 注册C++类型到QML
    qmlRegisterType<smartsched::client::DepartmentModel>("SmartSched.Models", 1, 0, "DepartmentModel");
    qmlRegisterType<smartsched::client::QueueModel>("SmartSched.Models", 1, 0, "QueueModel");
    qmlRegisterType<smartsched::client::PatientModel>("SmartSched.Models", 1, 0, "PatientModel");
    qmlRegisterType<smartsched::client::DoctorModel>("SmartSched.Models", 1, 0, "DoctorModel");
    
    // 暴露单例到QML
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("networkClient", networkClient);
    context->setContextProperty("departmentModel", departmentModel);
    context->setContextProperty("queueModel", queueModel);
    context->setContextProperty("patientModel", patientModel);
    context->setContextProperty("doctorModel", doctorModel);
    
    // 暴露枚举
    context->setContextProperty("APP_VERSION", QString(SMARTSCHED_VERSION_STRING));
    context->setContextProperty("HOSPITAL_NAME", QString("智序医院"));
    
    // 加载主QML文件
    const QUrl url(QStringLiteral("qrc:/qml/patient/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    // 自动连接服务器
    networkClient->connectToServer();
    
    return app.exec();
}
