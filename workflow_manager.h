#ifndef WORKFLOW_MANAGER_H
#define WORKFLOW_MANAGER_H

#include <QObject>
#include <QList>

struct Workflow {
    int id;
    QString name;
    QString description;
    QString command;
    QString iconPath;
    int sortOrder;
    QString createdAt;
};

class WorkflowManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkflowManager(QObject *parent = nullptr);

    bool addWorkflow(const QString &name, const QString &command, const QString &description = "", const QString &iconPath = "");
    bool updateWorkflow(int id, const QString &name, const QString &command, const QString &description = "", const QString &iconPath = "");
    bool deleteWorkflow(int id);
    bool updateSortOrder(int id, int sortOrder);
    Workflow getWorkflow(int id);
    QList<Workflow> getAllWorkflows();
    bool launchWorkflow(int id);
    bool launchWorkflowByCommand(const QString &command);

private:
    void initDatabase();
};

#endif // WORKFLOW_MANAGER_H
