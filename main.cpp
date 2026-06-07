#include "mainwindow.h"
#include "database_manager.h"
#include <QFile>
#include <QApplication>
#include <QIcon>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QLocalServer>

static const char *SERVER_NAME = "OrbitDesk_SingleInstance_IPC";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 尝试连接已运行的实例，让它恢复窗口
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);
    if (socket.waitForConnected(500)) {
        socket.write("show");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    // 单实例锁
    QSharedMemory sharedMemory("OrbitDesk_SingleInstance");
    if (!sharedMemory.create(1)) {
        return 0;
    }
    a.setWindowIcon(QIcon(":/resources/icons/app.png"));
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
