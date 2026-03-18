#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>

class OverviewPage;
class TasksPage;
class AlertsPage;
class SettingsPage;

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
    void appendLog(const QString &text);

private:
    Ui::MainWindow *ui;

    OverviewPage *m_overviewPage;
    TasksPage *m_tasksPage;
    AlertsPage *m_alertsPage;
    SettingsPage *m_settingsPage;

    bool m_isMaximized = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // MAINWINDOW_H