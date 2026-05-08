#include "mainwindow.h"
#include "database_manager.h"
#include <QFile>
#include <QApplication>
#include <QIcon>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 单实例检测
    QSharedMemory sharedMemory("OrbitDesk_SingleInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::information(nullptr, "OrbitDesk",
            "程序已在运行中，请检查系统托盘区域。");
        return 0;
    }

    a.setWindowIcon(QIcon(":/icons/app.png"));
    QFile styleFile(":/dark.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = styleFile.readAll();
        a.setStyleSheet(style);
        styleFile.close();
    }
    DatabaseManager::instance().initDatabase();
    MainWindow w;
    w.show();
    return a.exec();
}
