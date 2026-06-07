#ifndef AI_CHAT_MANAGER_H
#define AI_CHAT_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QMap>

class AiChatManager : public QObject
{
    Q_OBJECT

public:
    explicit AiChatManager(QObject *parent = nullptr);

    void sendMessage(const QString &message, int sessionId = -1);
    void clearHistory();
    void addHistoryMessage(const QString &role, const QString &content);

    void setCurrentSession(int sessionId);
    int currentSessionId() const;

signals:
    void replyReceived(const QString &reply, int sessionId);
    void replyChunkReceived(const QString &chunk, int sessionId);
    void errorOccurred(const QString &error, int sessionId);
    void replyStarted(int sessionId);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void processStreamChunk(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_apiUrl;
    int m_currentSessionId = -1;
    QMap<QNetworkReply*, int> m_pendingRequests;  // reply -> sessionId
    QMap<int, QJsonArray> m_sessionMessages;      // sessionId -> 消息历史
    QMap<QNetworkReply*, QString> m_streamBuffers; // reply -> 已累积的完整内容
};

#endif // AI_CHAT_MANAGER_H
