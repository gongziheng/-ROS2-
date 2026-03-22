#ifndef DASHBOARDCLIENT_H
#define DASHBOARDCLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QUrl>
#include <QWebSocket>

#include <QElapsedTimer>
#include <QTimer>

class QNetworkAccessManager;

// 负责网络通信
class DashboardClient : public QObject
{
    Q_OBJECT

public:
    explicit DashboardClient(QObject *parent = nullptr);
    
    // websocket 连接
    void connectWebSocket(const QUrl &url);
    void disconnectWebSocket();

    // 调用dashboardapi http Get接口 获取当前状态
    void requestCurrentStatus(const QUrl &url);
    // 调用post接口 发布当前任务
    void submitTask(const QUrl &url, const QString &robotId, double targetX, double targetY, const QString &taskType);

signals:
    void statusUpdated(const QJsonObject &status);
    void taskSubmitted(const QJsonObject &result);

    void connectionChanged(bool connected);
    void logMessage(const QString &text);

private:
    // 处理获取的当前状态
    void handleStatusPayload(const QByteArray &payload);
    // 处理发布任务的回调
    void handleTaskPayload(const QByteArray &payload);

private:
    QNetworkAccessManager *m_http = nullptr;
    QWebSocket m_ws;
    QTimer *m_watchdog = nullptr;
    QElapsedTimer m_lastMessageTimer;
    bool m_connected = false;
};




#endif // DASHBOARDCLIENT_H