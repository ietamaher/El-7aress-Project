#include "ui/mainwindow.h"

#include <QApplication>
#include "core/systemcontroller.h"
#include <QFile>
#include <QDateTime>
#include <QDir>
#include "TimestampLogger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SystemController sysCtrl;
    sysCtrl.initializeSystem();
    sysCtrl.showMainWindow();

    return app.exec();
}
