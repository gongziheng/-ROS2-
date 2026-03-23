#ifndef DASHBOARDCLIENT_H
#define DASHBOARDCLIENT_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QWebSocket>
#include <QElapsedTimer>
#include <QTimer>

class QNetworkAccessManager;

class DashboardClient : public QObject
{
    Q_OBJECT

public:
    explicit DashboardClient(QObject *parent = nullptr);

    void connectWebSocket(const QUrl &url);
    void disconnectWebSocket();

    void requestCurrentStatus(const QUrl &url);
    void requestRecentTasks(const QUrl &url);
    void requestRecentAlerts(const QUrl &url);

    void submitTask(const QUrl &url,
                    const QString &robotId,
                    double targetX,
                    double targetY,
                    const QString &taskType);

signals:
    void statusUpdated(const QJsonObject &status);
    void recentTasksUpdated(const QJsonArray &items);
    void recentAlertsUpdated(const QJsonArray &items);
    void taskSubmitted(const QJsonObject &result);

    void connectionChanged(bool connected);
    void logMessage(const QString &text);

private:
    void handleStatusPayload(const QByteArray &payload);
    void handleTaskPayload(const QByteArray &payload);
    void handleRecentTasksPayload(const QByteArray &payload);
    void handleRecentAlertsPayload(const QByteArray &payload);

    void emitOfflineStatus(const QString &reason);
    void tryReconnect();
    void scheduleReconnect(const QString &reason);

private:
    QNetworkAccessManager *m_http = nullptr;
    QWebSocket m_ws;
    QTimer *m_watchdog = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QElapsedTimer m_lastMessageTimer;

    QUrl m_statusUrl;
    QUrl m_wsUrl;
    QJsonObject m_lastStatus;
    bool m_connected = false;
    bool m_connecting = false;
};

#endif // DASHBOARDCLIENT_H