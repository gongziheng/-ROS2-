#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "overviewpage.h"
#include "taskspage.h"
#include "alertspage.h"
#include "settingspage.h"

#include <QButtonGroup>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setMinimumSize(900, 620);

    setupPages();
    setupConnections();
    setupWindowBehavior();
    adaptToScreen();

    syncNavButtons(ui->btnNavOverview);
    ui->stackedPages->setCurrentWidget(m_overviewPage);

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

    ui->stackedPages->addWidget(m_overviewPage);
    ui->stackedPages->addWidget(m_tasksPage);
    ui->stackedPages->addWidget(m_alertsPage);
    ui->stackedPages->addWidget(m_settingsPage);
}

void MainWindow::setupConnections()
{
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    m_navGroup->addButton(ui->btnNavOverview);
    m_navGroup->addButton(ui->btnNavTasks);
    m_navGroup->addButton(ui->btnNavAlerts);
    m_navGroup->addButton(ui->btnNavSettings);

    connect(ui->btnNavOverview, &QPushButton::clicked,
            this, &MainWindow::onNavOverviewClicked);
    connect(ui->btnNavTasks, &QPushButton::clicked,
            this, &MainWindow::onNavTasksClicked);
    connect(ui->btnNavAlerts, &QPushButton::clicked,
            this, &MainWindow::onNavAlertsClicked);
    connect(ui->btnNavSettings, &QPushButton::clicked,
            this, &MainWindow::onNavSettingsClicked);

    connect(ui->btnWindowMinimize, &QPushButton::clicked,
            this, &MainWindow::onWindowMinClicked);
    connect(ui->btnWindowMaximize, &QPushButton::clicked,
            this, &MainWindow::onWindowMaxClicked);
    connect(ui->btnWindowClose, &QPushButton::clicked,
            this, &MainWindow::onWindowCloseClicked);
}

void MainWindow::setupWindowBehavior()
{
    // 1) 标题栏本体
    ui->titleBar->installEventFilter(this);

    // 2) 标题栏里面所有非按钮子控件，也都纳入拖动区域
    const auto titleChildren = ui->titleBar->findChildren<QWidget *>();
    for (QWidget *w : titleChildren) {
        if (w == ui->btnWindowMinimize ||
            w == ui->btnWindowMaximize ||
            w == ui->btnWindowClose) {
            continue;
        }
        w->installEventFilter(this);
    }
}

void MainWindow::adaptToScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        resize(1280, 800);
        return;
    }

    const QRect available = screen->availableGeometry();

    int targetWidth = qMin(1440, int(available.width() * 0.92));
    int targetHeight = qMin(900, int(available.height() * 0.90));

    targetWidth = qMax(900, qMin(targetWidth, available.width()));
    targetHeight = qMax(620, qMin(targetHeight, available.height()));

    resize(targetWidth, targetHeight);
    move(available.center() - rect().center());
}

bool MainWindow::isWindowControlButton(QObject *obj) const
{
    return obj == ui->btnWindowMinimize ||
           obj == ui->btnWindowMaximize ||
           obj == ui->btnWindowClose;
}

bool MainWindow::isTitleDragTarget(QObject *obj) const
{
    QWidget *w = qobject_cast<QWidget *>(obj);
    if (!w || !ui->titleBar) {
        return false;
    }

    if (isWindowControlButton(obj)) {
        return false;
    }

    return (w == ui->titleBar) || ui->titleBar->isAncestorOf(w);
}

void MainWindow::syncNavButtons(QPushButton *activeButton)
{
    if (activeButton) {
        activeButton->setChecked(true);
    }
}

void MainWindow::appendLog(const QString &text)
{
    statusBar()->showMessage(text, 3000);
}

void MainWindow::onNavOverviewClicked()
{
    ui->stackedPages->setCurrentWidget(m_overviewPage);
    syncNavButtons(ui->btnNavOverview);
    appendLog("Switch to Overview page.");
}

void MainWindow::onNavTasksClicked()
{
    ui->stackedPages->setCurrentWidget(m_tasksPage);
    syncNavButtons(ui->btnNavTasks);
    appendLog("Switch to Tasks page.");
}

void MainWindow::onNavAlertsClicked()
{
    ui->stackedPages->setCurrentWidget(m_alertsPage);
    syncNavButtons(ui->btnNavAlerts);
    appendLog("Switch to Alerts page.");
}

void MainWindow::onNavSettingsClicked()
{
    ui->stackedPages->setCurrentWidget(m_settingsPage);
    syncNavButtons(ui->btnNavSettings);
    appendLog("Switch to Settings page.");
}

void MainWindow::onWindowMinClicked()
{
    showMinimized();
}

void MainWindow::onWindowMaxClicked()
{
    if (windowState() & Qt::WindowMaximized) {
        setWindowState(Qt::WindowNoState);
        ui->btnWindowMaximize->setText("□");
        appendLog("Window restored.");
    } else {
        setWindowState(Qt::WindowMaximized);
        ui->btnWindowMaximize->setText("❐");
        appendLog("Window maximized.");
    }
}

void MainWindow::onWindowCloseClicked()
{
    close();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (!isTitleDragTarget(watched)) {
        return QMainWindow::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_point = mouseEvent->globalPos();

            if (!(windowState() & Qt::WindowMaximized)) {
                m_pressed = true;
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (m_pressed && (mouseEvent->buttons() & Qt::LeftButton)) {
            const QPoint movePoint = mouseEvent->globalPos() - m_point;
            const QPoint widgetPos = this->pos() + movePoint;
            m_point = mouseEvent->globalPos();
            this->move(widgetPos);
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        m_pressed = false;
        return true;
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        onWindowMaxClicked();
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}