#include "ai_chat_manager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QProcessEnvironment>

AiChatManager::AiChatManager(QObject *parent)
    : QObject(parent)
{
    // 从环境变量读取 API 配置
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_apiKey = env.value("ORBITDESK_API_KEY", "");
    m_apiUrl = env.value("ORBITDESK_API_URL", "https://token-plan-cn.xiaomimimo.com/v1/chat/completions");

    if (m_apiKey.isEmpty()) {
        qWarning() << "API Key 未设置! 请设置环境变量 ORBITDESK_API_KEY";
    }
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &AiChatManager::onReplyFinished);
    // 修复 SSL 问题
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    QSslConfiguration::setDefaultConfiguration(sslConfig);
    m_networkManager->setTransferTimeout(30000);
}

void AiChatManager::sendMessage(const QString &message)
{
    // 把用户消息加入对话历史
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message;
    m_messages.append(userMsg);

    // 构建请求体
    QJsonObject body;
    body["model"] = "mimo-v2.5-pro";

    body["messages"] = m_messages;

    // 构建 HTTP 请求
    QNetworkRequest request = QNetworkRequest(QUrl(m_apiUrl));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());


    // 发送请求
    emit replyStarted();
    qDebug() << "API URL:" << m_apiUrl;

    m_networkManager->post(request, QJsonDocument(body).toJson());
}

void AiChatManager::onReplyFinished(QNetworkReply *reply)
{
    qDebug() << "=== AI Reply Finished ===";
    qDebug() << "Error:" << reply->error();
    qDebug() << "Error string:" << reply->errorString();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Response data:" << data;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    QString content = obj["choices"][0]["message"]["content"].toString();
    qDebug() << "Parsed content:" << content;

    QJsonObject assistantMsg;
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = content;
    m_messages.append(assistantMsg);

    emit replyReceived(content);
    reply->deleteLater();
}
void AiChatManager::addHistoryMessage(const QString &role, const QString &content)
{
    QJsonObject msg;
    msg["role"] = role;
    msg["content"] = content;
    m_messages.append(msg);
}


void AiChatManager::clearHistory()
{
    m_messages = QJsonArray();
}
