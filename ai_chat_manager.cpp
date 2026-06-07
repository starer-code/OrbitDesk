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
    m_networkManager->setTransferTimeout(120000);  // 120秒，长文本生成需要时间
}

// 估算消息的 token 数量
static int estimateTokens(const QJsonObject &msg)
{
    QString content = msg["content"].toString();
    int charCount = content.length();
    // 中文约 1.5 token/字，英文约 0.75 token/word，取折中值
    return static_cast<int>(charCount * 0.8);
}

// 裁剪消息历史，防止超出上下文窗口
static QJsonArray trimMessages(const QJsonArray &messages, int maxTokens = 6000)
{
    if (messages.size() <= 2) return messages;

    // 计算总 token
    int totalTokens = 0;
    for (const auto &msg : messages) {
        totalTokens += estimateTokens(msg.toObject());
    }

    if (totalTokens <= maxTokens) return messages;

    // 超出阈值，保留首条 + 最近的消息
    QJsonArray trimmed;
    trimmed.append(messages.first());  // 保留第一条作为上下文

    // 从后往前保留消息，直到接近 token 上限
    int keepCount = 0;
    int usedTokens = estimateTokens(messages.first().toObject());
    for (int i = messages.size() - 1; i >= 1; --i) {
        int tokens = estimateTokens(messages[i].toObject());
        if (usedTokens + tokens > maxTokens - 100) break;  // 留 100 token 余量
        usedTokens += tokens;
        keepCount++;
    }

    // 插入省略标记
    QJsonObject marker;
    marker["role"] = "system";
    marker["content"] = "[历史消息已省略，以下是最近的对话]";
    trimmed.append(marker);

    // 加入最近的消息
    int startIndex = messages.size() - keepCount;
    for (int i = startIndex; i < messages.size(); ++i) {
        trimmed.append(messages[i]);
    }

    return trimmed;
}

void AiChatManager::sendMessage(const QString &message, int sessionId)
{
    // 获取当前会话的消息历史
    QJsonArray &messages = m_sessionMessages[sessionId];

    // 把用户消息加入对话历史
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message;
    messages.append(userMsg);

    // 裁剪消息历史，防止超出上下文窗口
    QJsonArray trimmedMessages = trimMessages(messages);

    // 构建请求体
    QJsonObject body;
    body["model"] = "mimo-v2.5-pro";
    body["messages"] = trimmedMessages;
    body["stream"] = true;  // 启用流式输出

    // 构建 HTTP 请求
    QNetworkRequest request = QNetworkRequest(QUrl(m_apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    // 发送请求，记录 sessionId
    emit replyStarted(sessionId);

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    m_pendingRequests[reply] = sessionId;
    m_streamBuffers[reply] = "";

    // 连接 readyRead 信号用于流式读取
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        processStreamChunk(reply);
    });
}

void AiChatManager::onReplyFinished(QNetworkReply *reply)
{
    int sessionId = m_pendingRequests.take(reply);
    QString fullContent = m_streamBuffers.take(reply);

    if (reply->error() != QNetworkReply::NoError) {
        // 如果已经有部分内容，说明是中途出错
        if (!fullContent.isEmpty()) {
            // 部分内容也算成功，追加到历史
            QJsonArray &messages = m_sessionMessages[sessionId];
            QJsonObject assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = fullContent;
            messages.append(assistantMsg);
            emit replyReceived(fullContent, sessionId);
        } else {
            emit errorOccurred(reply->errorString(), sessionId);
        }
        reply->deleteLater();
        return;
    }

    // 流式模式下，内容已经在 processStreamChunk 中处理
    // 这里只需发送最终完成信号
    if (!fullContent.isEmpty()) {
        // 确保内容已追加到历史（如果还没追加的话）
        QJsonArray &messages = m_sessionMessages[sessionId];
        if (messages.isEmpty() || messages.last().toObject()["content"].toString() != fullContent) {
            QJsonObject assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = fullContent;
            messages.append(assistantMsg);
        }
        emit replyReceived(fullContent, sessionId);
    } else {
        // 非流式模式的兼容处理
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        QJsonArray choices = obj["choices"].toArray();
        if (choices.isEmpty()) {
            emit errorOccurred("API 返回数据格式异常", sessionId);
            reply->deleteLater();
            return;
        }

        QString content = choices[0].toObject()["message"].toObject()["content"].toString();
        if (content.isEmpty()) {
            emit errorOccurred("API 返回内容为空", sessionId);
            reply->deleteLater();
            return;
        }

        QJsonArray &messages = m_sessionMessages[sessionId];
        QJsonObject assistantMsg;
        assistantMsg["role"] = "assistant";
        assistantMsg["content"] = content;
        messages.append(assistantMsg);

        emit replyReceived(content, sessionId);
    }

    reply->deleteLater();
}

void AiChatManager::processStreamChunk(QNetworkReply *reply)
{
    int sessionId = m_pendingRequests.value(reply, -1);
    if (sessionId < 0) return;

    QByteArray data = reply->readAll();
    QString buffer = QString::fromUtf8(data);

    // 解析 SSE 格式: data: {...}\n\n
    QStringList lines = buffer.split('\n');
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || !trimmed.startsWith("data: ")) continue;

        QString jsonStr = trimmed.mid(6); // 去掉 "data: " 前缀
        if (jsonStr == "[DONE]") continue;

        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull()) continue;

        QJsonObject obj = doc.object();
        QJsonArray choices = obj["choices"].toArray();
        if (choices.isEmpty()) continue;

        QJsonObject delta = choices[0].toObject()["delta"].toObject();
        QString content = delta["content"].toString();

        if (!content.isEmpty()) {
            m_streamBuffers[reply] += content;
            emit replyChunkReceived(content, sessionId);
        }
    }
}
void AiChatManager::addHistoryMessage(const QString &role, const QString &content)
{
    QJsonObject msg;
    msg["role"] = role;
    msg["content"] = content;
    m_sessionMessages[m_currentSessionId].append(msg);
}


void AiChatManager::clearHistory()
{
    m_sessionMessages[m_currentSessionId] = QJsonArray();
}

void AiChatManager::setCurrentSession(int sessionId)
{
    m_currentSessionId = sessionId;
}

int AiChatManager::currentSessionId() const
{
    return m_currentSessionId;
}
