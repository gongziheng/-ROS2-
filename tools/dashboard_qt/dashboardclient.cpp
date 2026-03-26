#include "dashboardclient.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

DashboardClient::DashboardClient(QObject *parent)
    : QObject(parent)
    , m_http(new QNetworkAccessManager(this))
    , m_watchdog(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_watchdog->setInterval(1000);

    m_reconnectTimer->setInterval(2000);
    m_reconnectTimer->setSingleShot(false);

    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        tryReconnect();
    });

    connect(m_watchdog, &QTimer::timeout, this, [this]() {
        if (m_connected &&
            m_lastMessageTimer.isValid() &&
            m_lastMessageTimer.elapsed() > 3500) {
            m_connected = false;
            m_connecting = false;

            emit connectionChanged(false);
            emit logMessage("WebSocket status timeout, fallback to HTTP polling.");

            m_ws.close();
            scheduleReconnect("watchdog timeout");
        }
    });

    m_watchdog->start();

    connect(&m_ws, &QWebSocket::connected, this, [this]() {
        m_connected = true;
        m_connecting = false;
        m_manualDisconnect = false;
        m_lastMessageTimer.restart();

        if (m_reconnectTimer->isActive()) {
            m_reconnectTimer->stop();
        }

        emit connectionChanged(true);
        emit logMessage("WebSocket connected.");

        if (m_statusUrl.isValid()) {
            requestCurrentStatus(m_statusUrl);
        }
    });

    connect(&m_ws, &QWebSocket::disconnected, this, [this]() {
        const bool wasAlive = m_connected || m_connecting;

        m_connected = false;
        m_connecting = false;

        if (m_manualDisconnect) {
            m_manualDisconnect = false;
            debugLog("WebSocket disconnected by client.");
            return;
        }

        if (wasAlive) {
            emit connectionChanged(false);
        }

        emit logMessage("WebSocket disconnected.");
        scheduleReconnect("websocket disconnected");
    });

    connect(&m_ws,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this,
            [this](QAbstractSocket::SocketError) {
                m_connecting = false;
                emit logMessage(QString("WebSocket error: %1").arg(m_ws.errorString()));

                if (!m_manualDisconnect &&
                    m_ws.state() == QAbstractSocket::UnconnectedState) {
                    scheduleReconnect(m_ws.errorString());
                }
            });

    connect(&m_ws, &QWebSocket::textMessageReceived, this, [this](const QString &message) {
        m_lastMessageTimer.restart();
        handleStatusPayload(message.toUtf8());
    });
}

void DashboardClient::setDebugEnabled(bool enabled)
{
    m_debugEnabled = enabled;
}

void DashboardClient::debugLog(const QString &text)
{
    if (m_debugEnabled) {
        emit logMessage(text);
    }
}

void DashboardClient::connectWebSocket(const QUrl &url)
{
    if (!url.isValid()) {
        emit logMessage("WebSocket URL invalid.");
        return;
    }

    m_wsUrl = url;
    m_manualDisconnect = false;

    if (m_ws.state() == QAbstractSocket::ConnectedState || m_connecting) {
        return;
    }

    emit logMessage(QString("Connecting WebSocket: %1").arg(url.toString()));
    m_connecting = true;
    m_ws.open(m_wsUrl);
}

void DashboardClient::disconnectWebSocket()
{
    m_reconnectTimer->stop();
    m_connecting = false;
    m_manualDisconnect = true;

    if (m_ws.state() != QAbstractSocket::UnconnectedState) {
        m_ws.close();
    }
}

void DashboardClient::requestCurrentStatus(const QUrl &url)
{
    if (!url.isValid()) {
        emit logMessage("GET /api/status skipped: invalid URL.");
        return;
    }

    m_statusUrl = url;

    QNetworkRequest request(url);
    QNetworkReply *reply = m_http->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || httpCode >= 400) {
            emit logMessage(QString("GET /api/status failed: http=%1, error=%2")
                                .arg(httpCode)
                                .arg(reply->errorString()));
            emitOfflineStatus("Dashboard API unavailable");
            scheduleReconnect("http status failed");
            reply->deleteLater();
            return;
        }

        handleStatusPayload(payload);
        reply->deleteLater();
    });
}

void DashboardClient::requestRecentTasks(const QUrl &url)
{
    if (!url.isValid()) {
        emit logMessage("GET /api/tasks/recent skipped: invalid URL.");
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply *reply = m_http->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || httpCode >= 400) {
            emit logMessage(QString("GET /api/tasks/recent failed: http=%1, error=%2")
                                .arg(httpCode)
                                .arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        handleRecentTasksPayload(payload);
        reply->deleteLater();
    });
}

void DashboardClient::requestRecentAlerts(const QUrl &url)
{
    if (!url.isValid()) {
        emit logMessage("GET /api/alerts/recent skipped: invalid URL.");
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply *reply = m_http->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || httpCode >= 400) {
            emit logMessage(QString("GET /api/alerts/recent failed: http=%1, error=%2")
                                .arg(httpCode)
                                .arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        handleRecentAlertsPayload(payload);
        reply->deleteLater();
    });
}

void DashboardClient::submitTask(const QUrl &url,
                                 const QString &robotId,
                                 double targetX,
                                 double targetY,
                                 const QString &taskType)
{
    if (!url.isValid()) {
        emit logMessage("POST /api/tasks skipped: invalid URL.");
        return;
    }

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
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || httpCode >= 400) {
            emit logMessage(QString("POST /api/tasks failed: http=%1, error=%2, body=%3")
                                .arg(httpCode)
                                .arg(reply->errorString())
                                .arg(QString::fromUtf8(responseBytes)));
            reply->deleteLater();
            return;
        }

        handleTaskPayload(responseBytes);
        reply->deleteLater();
    });
}

void DashboardClient::handleStatusPayload(const QByteArray &payload)
{
    debugLog(QString("[DEBUG] raw payload: %1").arg(QString::fromUtf8(payload)));

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("Status JSON parse failed: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject root = doc.object();

    if (root.contains("data") && root.value("data").isObject()) {
        m_lastStatus = root.value("data").toObject();
        debugLog(QString("[DEBUG] parsed status from http: %1")
                     .arg(QString::fromUtf8(
                         QJsonDocument(m_lastStatus).toJson(QJsonDocument::Compact))));
        emit statusUpdated(m_lastStatus);
    } else {
        m_lastStatus = root;
        debugLog(QString("[DEBUG] parsed status from ws: %1")
                     .arg(QString::fromUtf8(
                         QJsonDocument(m_lastStatus).toJson(QJsonDocument::Compact))));
        emit statusUpdated(m_lastStatus);
    }
}

void DashboardClient::handleRecentTasksPayload(const QByteArray &payload)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("Tasks JSON parse failed: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject root = doc.object();
    if (root.contains("data") && root.value("data").isArray()) {
        emit recentTasksUpdated(root.value("data").toArray());
    }
}

void DashboardClient::handleRecentAlertsPayload(const QByteArray &payload)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QString("Alerts JSON parse failed: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject root = doc.object();
    if (root.contains("data") && root.value("data").isArray()) {
        emit recentAlertsUpdated(root.value("data").toArray());
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

void DashboardClient::emitOfflineStatus(const QString &reason)
{
    debugLog(QString("[DEBUG] emitOfflineStatus reason=%1").arg(reason));

    QJsonObject status = m_lastStatus;
    if (!status.contains("robot_id")) status["robot_id"] = "robot_1";
    if (!status.contains("x")) status["x"] = 0.0;
    if (!status.contains("y")) status["y"] = 0.0;
    if (!status.contains("yaw")) status["yaw"] = 0.0;
    if (!status.contains("linear_vel")) status["linear_vel"] = 0.0;
    if (!status.contains("angular_vel")) status["angular_vel"] = 0.0;
    if (!status.contains("battery_pct")) status["battery_pct"] = 0.0;
    if (!status.contains("task_id")) status["task_id"] = "";

    status["is_online"] = false;
    status["mode"] = "offline";
    status["alert_text"] = reason;

    emit statusUpdated(status);
}

void DashboardClient::scheduleReconnect(const QString &reason)
{
    if (!m_reconnectTimer->isActive()) {
        emit logMessage(QString("Start reconnect loop: %1").arg(reason));
        m_reconnectTimer->start();
    }
}

void DashboardClient::tryReconnect()
{
    if (m_statusUrl.isValid()) {
        requestCurrentStatus(m_statusUrl);
    }

    if (m_wsUrl.isValid() &&
        m_ws.state() == QAbstractSocket::UnconnectedState &&
        !m_connecting) {
        emit logMessage(QString("Retry WebSocket: %1").arg(m_wsUrl.toString()));
        m_connecting = true;
        m_ws.open(m_wsUrl);
    }
}