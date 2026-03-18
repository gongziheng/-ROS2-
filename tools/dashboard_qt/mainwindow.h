#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>

class QButtonGroup;
class QPushButton;

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

    QButtonGroup *m_navGroup = nullptr;

    bool m_pressed = false;
    QPoint m_point;
};

#endif // MAINWINDOW_H