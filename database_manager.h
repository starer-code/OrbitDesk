#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    // 获取单例
    static DatabaseManager& instance();

    // 获取数据库连接
    QSqlDatabase database();

    // 初始化数据库（建表）
    void initDatabase();

    // 关闭数据库
    void close();

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    // 禁止拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
    bool m_initialized = false;
};

#endif // DATABASE_MANAGER_H
