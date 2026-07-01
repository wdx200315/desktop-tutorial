#include <QFile>
#include <QApplication>
#include <QStyleFactory>
#include "LoginDialog.h"
#include "AdminMainWindow.h"
#include "NetworkManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    // 加载样式
    QFile styleFile(":/resources/styles/admin_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    NetworkManager net;
    LoginDialog login(&net);
    if (login.exec() == QDialog::Accepted) {
        AdminMainWindow w(&net);
        w.show();
        return app.exec();
    }
    return 0;
}
