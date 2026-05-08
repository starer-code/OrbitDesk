#include "task_manager.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

TaskManager::TaskManager(QObject *parent)
    : QObject(parent)
{
    ensureTableExists();
}

void TaskManager::ensureTableExists()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "    id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title       TEXT NOT NULL,"
        "    description TEXT,"
        "    priority    TEXT DEFAULT 'medium',"
        "    category    TEXT,"
        "    due_date    DATE,"
        "    is_done     INTEGER DEFAULT 0,"
        "    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
        );
}

    bool TaskManager::addTask(const QString &title, const QString &priority, const QDateTime &dueDate, const QString &category)

{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO tasks (title, priority, due_date, category) "
        "VALUES (?, ?, ?, ?)"
        );
    query.addBindValue(title);
    query.addBindValue(priority);
    query.addBindValue(dueDate.toString("yyyy-MM-dd HH:mm"));
    query.addBindValue(category);

    if (!query.exec()) {
        qDebug() << "Add task failed:" << query.lastError().text();
        return false;
    }
    qDebug() << "Task added, id:" << query.lastInsertId().toInt();
    return true;
}

bool TaskManager::deleteTask(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM tasks WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Delete task failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool TaskManager::toggleDone(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET is_done = CASE WHEN is_done = 0 THEN 1 ELSE 0 END WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Toggle task failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<Task> TaskManager::getAllTasks()
{
    QList<Task> tasks;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return tasks;

    QSqlQuery query(db);
    query.exec("SELECT id, title, description, priority, category, due_date, is_done, created_at FROM tasks ORDER BY is_done ASC, created_at DESC");

    while (query.next()) {
        Task task;
        task.id = query.value(0).toInt();
        task.title = query.value(1).toString();
        task.description = query.value(2).toString();
        task.priority = query.value(3).toString();
        task.category = query.value(4).toString();
        task.dueDate = QDateTime::fromString(query.value(5).toString(), "yyyy-MM-dd HH:mm");


        task.isDone = query.value(6).toBool();
        task.createdAt = QDateTime::fromString(query.value(7).toString(), "yyyy-MM-dd HH:mm:ss");
        tasks.append(task);
    }
    return tasks;
}

int TaskManager::taskCount()
{
    return getAllTasks().size();
}
