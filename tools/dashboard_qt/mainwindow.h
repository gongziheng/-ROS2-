#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QUrl>

class QButtonGroup;
class QPushButton;
class QTimer;

class OverviewPage;
class TasksPage;
class AlertsPage;
class SettingsPage;
class DashboardClient;

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
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

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
    void adaptToScreen();

    bool isWindowControlButton(QObject *obj) const;
    bool isTitleDragTarget(QObject *obj) const;
    void syncNavButtons(QPushButton *activeButton);
    void appendLog(const QString &text);

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

    bool m_pressed = false;
    QPoint m_point;

    QUrl m_statusUrl = QUrl("http://127.0.0.1:8000/api/status");
    QUrl m_taskUrl = QUrl("http://127.0.0.1:8000/api/tasks");
    QUrl m_recentTasksUrl = QUrl("http://127.0.0.1:8000/api/tasks/recent");
    QUrl m_recentAlertsUrl = QUrl("http://127.0.0.1:8000/api/alerts/recent");
    QUrl m_wsUrl = QUrl("ws://127.0.0.1:8000/ws/status");
};

#endif // MAINWINDOW_H