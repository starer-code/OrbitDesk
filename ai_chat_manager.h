#ifndef AI_CHAT_MANAGER_H
#define AI_CHAT_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

class AiChatManager : public QObject
{
    Q_OBJECT

public:
    explicit AiChatManager(QObject *parent = nullptr);

    void sendMessage(const QString &message);
    void clearHistory();
    void addHistoryMessage(const QString &role, const QString &content);


signals:
    void replyReceived(const QString &reply);
    void errorOccurred(const QString &error);
    void replyStarted();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QJsonArray m_messages;
    QString m_apiKey;
    QString m_apiUrl;
};

#endif // AI_CHAT_MANAGER_H
