#include "chat_session_manager.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

ChatSessionManager::ChatSessionManager(QObject *parent)
    : QObject(parent)
{
}

int ChatSessionManager::createSession(const QString &title)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return -1;

    QSqlQuery q(db);
    q.prepare("INSERT INTO chat_sessions (title) VALUES (?)");
    q.addBindValue(title);
    if (!q.exec()) {
        qDebug() << "Create session error:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ChatSessionManager::deleteSession(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);

    // 删除关联的聊天记录
    q.prepare("DELETE FROM chat_history WHERE session_id = ?");
    q.addBindValue(QString::number(id));
    q.exec();

    // 删除会话
    q.prepare("DELETE FROM chat_sessions WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec()) {
        qDebug() << "Delete session error:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChatSessionManager::renameSession(int id, const QString &newTitle)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("UPDATE chat_sessions SET title = ?, updated_at = datetime('now') WHERE id = ?");
    q.addBindValue(newTitle);
    q.addBindValue(id);
    if (!q.exec()) {
        qDebug() << "Rename session error:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChatSessionManager::updateSessionTimestamp(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("UPDATE chat_sessions SET updated_at = datetime('now') WHERE id = ?");
    q.addBindValue(id);
    if (!q.exec()) {
        qDebug() << "Update session timestamp error:" << q.lastError().text();
        return false;
    }
    return true;
}

ChatSession ChatSessionManager::getSession(int id)
{
    ChatSession session;
    session.id = -1;

    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return session;

    QSqlQuery q(db);
    q.prepare("SELECT id, title, created_at, updated_at FROM chat_sessions WHERE id = ?");
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        session.id = q.value(0).toInt();
        session.title = q.value(1).toString();
        session.createdAt = q.value(2).toString();
        session.updatedAt = q.value(3).toString();
    }
    return session;
}

QList<ChatSession> ChatSessionManager::getAllSessions()
{
    QList<ChatSession> sessions;

    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return sessions;

    QSqlQuery q(db);
    q.exec("SELECT id, title, created_at, updated_at FROM chat_sessions ORDER BY updated_at DESC");

    while (q.next()) {
        ChatSession s;
        s.id = q.value(0).toInt();
        s.title = q.value(1).toString();
        s.createdAt = q.value(2).toString();
        s.updatedAt = q.value(3).toString();
        sessions.append(s);
    }
    return sessions;
}
