#include "dashboardclient.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

DashboardClient::DashboardClient(QObject *parent)
    : QObject(parent)
    , m_http(new QNetworkAccessManager(this))
{
    connect(&m_ws, &QWebSocket::connected, this, [this]() {
        emit connectionChanged(true);
        emit logMessage("WebSocket connected.");
    });

    connect(&m_ws, &QWebSocket::disconnected, this, [this]() {
        emit connectionChanged(false);
        emit logMessage("WebSocket disconnected.");
    });

    connect(&m_ws, &QWebSocket::textMessageReceived,
            this,
            [this](const QString &message) {
                handleStatusPayload(message.toUtf8());
            });

    connect(&m_ws,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            [this](QAbstractSocket::SocketError) {
                emit logMessage(QString("WebSocket error: %1").arg(m_ws.errorString()));
            });
}

void DashboardClient::connectWebSocket(const QUrl &url)
{
    emit logMessage(QString("Connecting WebSocket: %1").arg(url.toString()));
    m_ws.open(url);
}

void DashboardClient::disconnectWebSocket()
{
    m_ws.close();
}

void DashboardClient::requestCurrentStatus(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = m_http->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage(QString("GET /api/status failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        handleStatusPayload(payload);
        reply->deleteLater();
    });
}

void DashboardClient::submitTask(const QUrl &url,
                                 const QString &robotId,
                                 double targetX,
                                 double targetY,
                                 const QString &taskType)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["robot_id"] = robotId;
    body["target_x"] = targetX;
    body["target_y"] = targetY;
    body["task_type"] = taskType;

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = m_http->post(request, payload);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray responseBytes = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage(QString("POST /api/tasks failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        handleTaskPayload(responseBytes);
        reply->deleteLater();
    });
}

void DashboardClient::handleStatusPayload(const QByteArray &payload)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("Status JSON parse failed: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject root = doc.object();

    // 兼容两种格式：
    // 1) /api/status -> { ok: true, data: {...} }
    // 2) /ws/status  -> {...}
    if (root.contains("data") && root.value("data").isObject()) {
        emit statusUpdated(root.value("data").toObject());
    } else {
        emit statusUpdated(root);
    }
}

void DashboardClient::handleTaskPayload(const QByteArray &payload)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("Task JSON parse failed: %1").arg(error.errorString()));
        return;
    }

    emit taskSubmitted(doc.object());
}