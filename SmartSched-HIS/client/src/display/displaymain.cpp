/**
 * @file displaymain.cpp
 * @brief 排队看板大屏 - 主程序入口
 * 
 * 功能：大屏幕显示排队叫号信息，支持全屏
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "smartsched/common/version.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // 应用信息
    app.setApplicationName("SmartSched-排队看板");
    app.setApplicationVersion(SMARTSCHED_VERSION_STRING);
    app.setOrganizationName("SmartSched Healthcare");
    
    // 创建QML引擎
    QQmlApplicationEngine engine;
    
    // 暴露常量到QML
    QQmlContext* context = engine.rootContext();
    context->setContextProperty("HOSPITAL_NAME", QString("智序医院"));
    context->setContextProperty("APP_VERSION", QString(SMARTSCHED_VERSION_STRING));
    
    // 加载QML
    const QUrl url(QStringLiteral("qrc:/qml/display/DisplayBoard.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    return app.exec();
}
