#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QUrl>

class QButtonGroup;
class QPushButton;
class QTimer;
class QResizeEvent;

class OverviewPage;
class TasksPage;
class AlertsPage;
class SettingsPage;
class DashboardClient;
class ResizeHandle;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onNavOverviewClicked();
    void onNavTasksClicked();
    void onNavAlertsClicked();
    void onNavSettingsClicked();
    void onWindowMinClicked();
    void onWindowMaxClicked();
    void onWindowCloseClicked();

private:
    void setupPages();
    void setupConnections();
    void setupWindowBehavior();
    void setupResizeHandles();
    void updateResizeHandlesGeometry();
    void adaptToScreen();

    bool isWindowControlButton(QObject *obj) const;
    bool isTitleDragTarget(QObject *obj) const;
    void syncNavButtons(QPushButton *activeButton);
    void appendLog(const QString &text);

    QString apiBaseUrlFromStatusUrl() const;
    void applyEndpoints(const QString &apiBaseUrl, const QString &wsUrl);
    void refreshDashboardData();

private:
    Ui::MainWindow *ui = nullptr;

    OverviewPage *m_overviewPage = nullptr;
    TasksPage *m_tasksPage = nullptr;
    AlertsPage *m_alertsPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;

    DashboardClient *m_client = nullptr;
    QButtonGroup *m_navGroup = nullptr;
    QTimer *m_statusTimer = nullptr;
    QTimer *m_historyTimer = nullptr;

    ResizeHandle *m_handleLeft = nullptr;
    ResizeHandle *m_handleTop = nullptr;
    ResizeHandle *m_handleRight = nullptr;
    ResizeHandle *m_handleBottom = nullptr;
    ResizeHandle *m_handleTopLeft = nullptr;
    ResizeHandle *m_handleTopRight = nullptr;
    ResizeHandle *m_handleBottomLeft = nullptr;
    ResizeHandle *m_handleBottomRight = nullptr;

    bool m_pressed = false;
    QPoint m_point;

    QUrl m_statusUrl = QUrl("http://127.0.0.1:8000/api/status");
    QUrl m_taskUrl = QUrl("http://127.0.0.1:8000/api/tasks");
    QUrl m_recentTasksUrl = QUrl("http://127.0.0.1:8000/api/tasks/recent");
    QUrl m_recentAlertsUrl = QUrl("http://127.0.0.1:8000/api/alerts/recent");
    QUrl m_wsUrl = QUrl("ws://127.0.0.1:8000/ws/status");
};

#endif // MAINWINDOW_H