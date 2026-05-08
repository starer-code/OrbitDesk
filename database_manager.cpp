#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

QSqlDatabase DatabaseManager::database()
{
    if (!m_db.isOpen()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE");

        // 数据库文件存在用户数据目录下
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataPath);
        QString dbPath = dataPath + "/orbitdesk.db";

        m_db.setDatabaseName(dbPath);
        qDebug() << "Database path:" << dbPath;

        if (!m_db.open()) {
            qDebug() << "Database open failed:" << m_db.lastError().text();
        } else {
            qDebug() << "Database opened successfully";
        }
    }
    return m_db;
}

void DatabaseManager::initDatabase()
{
    if (m_initialized) return;

    QSqlDatabase db = database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);

    // 任务表
    query.exec(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title       TEXT NOT NULL,"
        "    description TEXT,"
        "    priority    TEXT DEFAULT 'medium',"
        "    category    TEXT,"
        "    due_date    DATETIME,"
        "    is_done     INTEGER DEFAULT 0,"
        "    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    // 笔记表
    query.exec(
        "CREATE TABLE IF NOT EXISTS notes ("
        "    id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title      TEXT,"
        "    content    TEXT,"
        "    tags       TEXT,"
        "    is_pinned  INTEGER DEFAULT 0,"
        "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    // 番茄钟记录表
    query.exec(
        "CREATE TABLE IF NOT EXISTS pomodoro_logs ("
        "    id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    duration_min INTEGER,"
        "    task_id      INTEGER,"
        "    started_at   DATETIME,"
        "    completed    INTEGER DEFAULT 1,"
        "    FOREIGN KEY (task_id) REFERENCES tasks(id)"
        ")"
        );

    // AI 对话记录表
    query.exec(
        "CREATE TABLE IF NOT EXISTS chat_history ("
        "    id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    role       TEXT,"
        "    content    TEXT,"
        "    session_id TEXT,"
        "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    // 用户配置表
    query.exec(
        "CREATE TABLE IF NOT EXISTS user_settings ("
        "    key   TEXT PRIMARY KEY,"
        "    value TEXT"
        ")"
        );

    // 系统监控历史记录表
    query.exec(
        "CREATE TABLE IF NOT EXISTS monitor_stats ("
        "    id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    cpu_percent    REAL,"
        "    memory_percent REAL,"
        "    memory_used_mb INTEGER,"
        "    disk_percent   REAL,"
        "    net_up_kbps    REAL,"
        "    net_down_kbps  REAL,"
        "    recorded_at    DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );

    // 清理 24 小时前的监控记录
    query.exec("DELETE FROM monitor_stats WHERE recorded_at < datetime('now', '-1 day')");

    m_initialized = true;
    qDebug() << "All tables created successfully";
}

void DatabaseManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
        qDebug() << "Database closed";
    }
}
