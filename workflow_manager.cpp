#include "workflow_manager.h"
#include "database_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

WorkflowManager::WorkflowManager(QObject *parent)
    : QObject(parent)
{
    initDatabase();
}

void WorkflowManager::initDatabase()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS workflows ("
        "    id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name       TEXT NOT NULL,"
        "    description TEXT,"
        "    command    TEXT NOT NULL,"
        "    icon_path  TEXT,"
        "    sort_order INTEGER DEFAULT 0,"
        "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    );

    // 检查是否需要添加默认工作流
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM workflows");
    if (countQuery.next() && countQuery.value(0).toInt() == 0) {
        // 添加默认工作流
        addWorkflow("记事本", "notepad.exe", "打开Windows记事本");
        addWorkflow("计算器", "calc.exe", "打开Windows计算器");
        addWorkflow("画图", "mspaint.exe", "打开Windows画图工具");
        addWorkflow("文件管理器", "explorer.exe", "打开文件资源管理器");
        addWorkflow("浏览器", "https://www.google.com", "打开默认浏览器");
        addWorkflow("VS Code", "code", "打开Visual Studio Code");
    }
}

bool WorkflowManager::addWorkflow(const QString &name, const QString &command, const QString &description, const QString &iconPath)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO workflows (name, command, description, icon_path, sort_order) "
        "VALUES (?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM workflows))"
    );
    query.addBindValue(name);
    query.addBindValue(command);
    query.addBindValue(description);
    query.addBindValue(iconPath);

    if (!query.exec()) {
        qDebug() << "Add workflow failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool WorkflowManager::updateWorkflow(int id, const QString &name, const QString &command, const QString &description, const QString &iconPath)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "UPDATE workflows SET name = ?, command = ?, description = ?, icon_path = ? WHERE id = ?"
    );
    query.addBindValue(name);
    query.addBindValue(command);
    query.addBindValue(description);
    query.addBindValue(iconPath);
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Update workflow failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool WorkflowManager::deleteWorkflow(int id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM workflows WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Delete workflow failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool WorkflowManager::updateSortOrder(int id, int sortOrder)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE workflows SET sort_order = ? WHERE id = ?");
    query.addBindValue(sortOrder);
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "Update sort order failed:" << query.lastError().text();
        return false;
    }
    return true;
}

Workflow WorkflowManager::getWorkflow(int id)
{
    Workflow workflow;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return workflow;

    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, command, icon_path, sort_order, created_at FROM workflows WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        workflow.id = query.value(0).toInt();
        workflow.name = query.value(1).toString();
        workflow.description = query.value(2).toString();
        workflow.command = query.value(3).toString();
        workflow.iconPath = query.value(4).toString();
        workflow.sortOrder = query.value(5).toInt();
        workflow.createdAt = query.value(6).toString();
    }
    return workflow;
}

QList<Workflow> WorkflowManager::getAllWorkflows()
{
    QList<Workflow> workflows;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return workflows;

    QSqlQuery query(db);
    query.exec("SELECT id, name, description, command, icon_path, sort_order, created_at FROM workflows ORDER BY sort_order ASC");

    while (query.next()) {
        Workflow workflow;
        workflow.id = query.value(0).toInt();
        workflow.name = query.value(1).toString();
        workflow.description = query.value(2).toString();
        workflow.command = query.value(3).toString();
        workflow.iconPath = query.value(4).toString();
        workflow.sortOrder = query.value(5).toInt();
        workflow.createdAt = query.value(6).toString();
        workflows.append(workflow);
    }
    return workflows;
}

bool WorkflowManager::launchWorkflow(int id)
{
    Workflow workflow = getWorkflow(id);
    if (workflow.id == 0) return false;
    return launchWorkflowByCommand(workflow.command);
}

bool WorkflowManager::launchWorkflowByCommand(const QString &command)
{
    if (command.isEmpty()) return false;

    // 判断是否为URL
    if (command.startsWith("http://") || command.startsWith("https://") || command.startsWith("www.")) {
        QString url = command;
        if (!url.startsWith("http")) url = "https://" + url;
        return QDesktopServices::openUrl(QUrl(url));
    }

    // 否则作为命令执行
    return QProcess::startDetached(command);
}
