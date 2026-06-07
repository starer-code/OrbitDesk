#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QObject>
#include <QList>
#include <QDate>
#include <QDateTime>

struct Task {
    int id = 0;
    QString title;
    QString description;
    QString priority;
    QString category;
    QDateTime dueDate;
    bool isDone = false;
    QDateTime createdAt;
};

class TaskManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = nullptr);

    // 增删改查
    bool addTask(const QString &title, const QString &priority,
    const QDateTime &dueDate, const QString &category);

    bool deleteTask(int id);
    bool toggleDone(int id);
    QList<Task> getAllTasks();

    // 获取任务数量
    int taskCount();

    // 获取某任务的累计专注时长（分钟）
    int getFocusMinutes(int taskId);
};

#endif // TASK_MANAGER_H
