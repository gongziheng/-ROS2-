#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "overviewpage.h"
#include "taskspage.h"
#include "alertspage.h"
#include "settingspage.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_overviewPage(nullptr)
    , m_tasksPage(nullptr)
    , m_alertsPage(nullptr)
    , m_settingsPage(nullptr)
{
    ui->setupUi(this);

    // 如果你的 mainwindow.ui 用的是自定义标题栏，通常需要无边框窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    setupPages();
    setupConnections();

    // 假设标题栏容器对象名叫 titleBar
    if (ui->titleBar) {
        ui->titleBar->installEventFilter(this);
    }

    appendLog("Dashboard Qt started.");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPages()
{
    m_overviewPage = new OverviewPage(this);
    m_tasksPage = new TasksPage(this);
    m_alertsPage = new AlertsPage(this);
    m_settingsPage = new SettingsPage(this);

    // 假设 mainwindow.ui 中有一个 QStackedWidget，objectName = stackedPages
    ui->stackedPages->addWidget(m_overviewPage);
    ui->stackedPages->addWidget(m_tasksPage);
    ui->stackedPages->addWidget(m_alertsPage);
    ui->stackedPages->addWidget(m_settingsPage);

    ui->stackedPages->setCurrentWidget(m_overviewPage);
}

void MainWindow::setupConnections()
{
    // 左侧导航按钮 objectName 请和你的 mainwindow.ui 保持一致
    connect(ui->btnNavOverview, &QPushButton::clicked,
            this, &MainWindow::onNavOverviewClicked);
    connect(ui->btnNavTasks, &QPushButton::clicked,
            this, &MainWindow::onNavTasksClicked);
    connect(ui->btnNavAlerts, &QPushButton::clicked,
            this, &MainWindow::onNavAlertsClicked);
    connect(ui->btnNavSettings, &QPushButton::clicked,
            this, &MainWindow::onNavSettingsClicked);

    // 右上角窗口按钮
    connect(ui->btnWindowMinimize, &QPushButton::clicked,
            this, &MainWindow::onWindowMinClicked);
    connect(ui->btnWindowMaximize, &QPushButton::clicked,
            this, &MainWindow::onWindowMaxClicked);
    connect(ui->btnWindowClose, &QPushButton::clicked,
            this, &MainWindow::onWindowCloseClicked);
}

void MainWindow::appendLog(const QString &text)
{
    // 假设 mainwindow.ui 中全局日志框 objectName = textGlobalLog
    // if (ui->textGlobalLog) {
    //     ui->textGlobalLog->appendPlainText(text);
    // }
    statusBar()->showMessage(text, 3000);
}

void MainWindow::onNavOverviewClicked()
{
    ui->stackedPages->setCurrentWidget(m_overviewPage);
    appendLog("Switch to Overview page.");
}

void MainWindow::onNavTasksClicked()
{
    ui->stackedPages->setCurrentWidget(m_tasksPage);
    appendLog("Switch to Tasks page.");
}

void MainWindow::onNavAlertsClicked()
{
    ui->stackedPages->setCurrentWidget(m_alertsPage);
    appendLog("Switch to Alerts page.");
}

void MainWindow::onNavSettingsClicked()
{
    ui->stackedPages->setCurrentWidget(m_settingsPage);
    appendLog("Switch to Settings page.");
}

void MainWindow::onWindowMinClicked()
{
    showMinimized();
}

void MainWindow::onWindowMaxClicked()
{
    if (m_isMaximized) {
        showNormal();
        m_isMaximized = false;
    } else {
        showMaximized();
        m_isMaximized = true;
    }
}

void MainWindow::onWindowCloseClicked()
{
    close();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragOffset = mouseEvent->globalPos() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (m_dragging && !m_isMaximized) {
                auto *mouseEvent = static_cast<QMouseEvent *>(event);
                move(mouseEvent->globalPos() - m_dragOffset);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
            return true;
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            onWindowMaxClicked();
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}