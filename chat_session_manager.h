#ifndef CHAT_SESSION_MANAGER_H
#define CHAT_SESSION_MANAGER_H

#include <QObject>
#include <QList>

struct ChatSession {
    int id;
    QString title;
    QString createdAt;
    QString updatedAt;
};

class ChatSessionManager : public QObject
{
    Q_OBJECT

public:
    explicit ChatSessionManager(QObject *parent = nullptr);

    int createSession(const QString &title = "新对话");
    bool deleteSession(int id);
    bool renameSession(int id, const QString &newTitle);
    bool updateSessionTimestamp(int id);
    ChatSession getSession(int id);
    QList<ChatSession> getAllSessions();
};

#endif // CHAT_SESSION_MANAGER_H
