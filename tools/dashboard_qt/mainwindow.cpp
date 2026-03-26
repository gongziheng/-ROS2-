#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "alertspage.h"
#include "dashboardclient.h"
#include "overviewpage.h"
#include "resizehandle.h"
#include "settingspage.h"
#include "taskspage.h"

#include <QButtonGroup>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QStatusBar>
#include <QTimer>
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

    m_settingsPage->setEndpoints(QUrl(apiBaseUrlFromStatusUrl()), m_wsUrl);
    m_settingsPage->setStatusMessage("当前连接配置已加载。", true);
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

    connect(m_settingsPage, &SettingsPage::endpointsApplied,
            this, [this](const QString &apiBaseUrl, const QString &wsUrl) {
                applyEndpoints(apiBaseUrl, wsUrl);
            });
}

void MainWindow::setupWindowBehavior()
{
    ui->titleBar->installEventFilter(this);
    const auto titleChildren = ui->titleBar->findChildren<QWidget *>();
    for (QWidget *w : titleChildren) {
        if (w == ui->btnWindowMinimize ||
            w == ui->btnWindowMaximize ||
            w == ui->btnWindowClose) {
            continue;
        }
        w->installEventFilter(this);
    }

    m_client = new DashboardClient(this);
    m_client->setDebugEnabled(false);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(1000);

    m_historyTimer = new QTimer(this);
    m_historyTimer->setInterval(1500);

    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        m_client->requestCurrentStatus(m_statusUrl);
    });

    connect(m_historyTimer, &QTimer::timeout, this, [this]() {
        QWidget *current = ui->stackedPages->currentWidget();
        if (current == m_tasksPage) {
            m_client->requestRecentTasks(m_recentTasksUrl);
        } else if (current == m_alertsPage) {
            m_client->requestRecentAlerts(m_recentAlertsUrl);
        } else {
            static bool toggle = false;
            if (toggle) {
                m_client->requestRecentTasks(m_recentTasksUrl);
            } else {
                m_client->requestRecentAlerts(m_recentAlertsUrl);
            }
            toggle = !toggle;
        }
    });

    connect(m_tasksPage, &TasksPage::submitRequested,
            this,
            [this](const QString &robotId, double x, double y, const QString &taskType) {
                appendLog(QString("提交任务: robot=%1, x=%2, y=%3, type=%4")
                              .arg(robotId)
                              .arg(x, 0, 'f', 2)
                              .arg(y, 0, 'f', 2)
                              .arg(taskType));

                m_tasksPage->appendLog("正在提交任务...");
                m_client->submitTask(m_taskUrl, robotId, x, y, taskType);
            });

    connect(m_client, &DashboardClient::statusUpdated,
            m_overviewPage, &OverviewPage::updateStatus);

    connect(m_client, &DashboardClient::recentTasksUpdated,
            m_tasksPage, &TasksPage::updateRecentTasks);

    connect(m_client, &DashboardClient::recentAlertsUpdated,
            m_alertsPage, &AlertsPage::updateRecentAlerts);

    connect(m_client, &DashboardClient::taskSubmitted,
            this,
            [this](const QJsonObject &result) {
                const bool apiOk = result.value("ok").toBool();
                const QJsonObject data = result.value("data").toObject();
                const bool accepted = data.value("accepted").toBool();
                const QString message = data.value("message").toString();
                const QString taskId = data.value("task_id").toString();

                const QString line = QString("任务返回: api_ok=%1, accepted=%2, task_id=%3, message=%4")
                                         .arg(apiOk ? "true" : "false")
                                         .arg(accepted ? "true" : "false")
                                         .arg(taskId)
                                         .arg(message);
                appendLog(line);

                refreshDashboardData();
            });

    connect(m_client, &DashboardClient::connectionChanged,
            this,
            [this](bool connected) {
                appendLog(connected ? "WebSocket 已连接" : "WebSocket 已断开");

                if (connected) {
                    if (m_statusTimer->isActive()) {
                        m_statusTimer->stop();
                    }
                    m_settingsPage->setStatusMessage("WebSocket 已连接，当前为实时模式。", true);
                } else {
                    m_client->requestCurrentStatus(m_statusUrl);
                    if (!m_statusTimer->isActive()) {
                        m_statusTimer->start();
                    }
                    m_settingsPage->setStatusMessage("WebSocket 已断开，已回退到 HTTP 轮询。", false);
                }
            });

    connect(m_client, &DashboardClient::logMessage,
            this,
            [this](const QString &text) {
                appendLog(text);
            });

    setupResizeHandles();
    refreshDashboardData();
    m_client->connectWebSocket(m_wsUrl);

    // 启动时先开轮询兜底，等 WS 真连上后自动停掉
    m_statusTimer->start();
    m_historyTimer->start();
}

void MainWindow::setupResizeHandles()
{
    if (m_handleLeft) {
        return;
    }

    m_handleLeft = new ResizeHandle(this, ResizeHandle::Left, this);
    m_handleTop = new ResizeHandle(this, ResizeHandle::Top, this);
    m_handleRight = new ResizeHandle(this, ResizeHandle::Right, this);
    m_handleBottom = new ResizeHandle(this, ResizeHandle::Bottom, this);
    m_handleTopLeft = new ResizeHandle(this, ResizeHandle::TopLeft, this);
    m_handleTopRight = new ResizeHandle(this, ResizeHandle::TopRight, this);
    m_handleBottomLeft = new ResizeHandle(this, ResizeHandle::BottomLeft, this);
    m_handleBottomRight = new ResizeHandle(this, ResizeHandle::BottomRight, this);

    updateResizeHandlesGeometry();
}

void MainWindow::updateResizeHandlesGeometry()
{
    if (!m_handleLeft) {
        return;
    }

    ResizeHandle *handles[] = {
        m_handleLeft, m_handleTop, m_handleRight, m_handleBottom,
        m_handleTopLeft, m_handleTopRight, m_handleBottomLeft, m_handleBottomRight
    };

    if (isMaximized() || isFullScreen()) {
        for (ResizeHandle *handle : handles) {
            if (handle) {
                handle->hide();
            }
        }
        return;
    }

    const int edge = 6;
    const int corner = 12;
    const int w = width();
    const int h = height();

    m_handleLeft->setGeometry(0, corner, edge, h - corner * 2);
    m_handleTop->setGeometry(corner, 0, w - corner * 2, edge);
    m_handleRight->setGeometry(w - edge, corner, edge, h - corner * 2);
    m_handleBottom->setGeometry(corner, h - edge, w - corner * 2, edge);

    m_handleTopLeft->setGeometry(0, 0, corner, corner);
    m_handleTopRight->setGeometry(w - corner, 0, corner, corner);
    m_handleBottomLeft->setGeometry(0, h - corner, corner, corner);
    m_handleBottomRight->setGeometry(w - corner, h - corner, corner, corner);

    for (ResizeHandle *handle : handles) {
        if (handle) {
            handle->show();
            handle->raise();
        }
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
    if (m_tasksPage) {
        m_tasksPage->appendLog(text);
    }
}

QString MainWindow::apiBaseUrlFromStatusUrl() const
{
    QString text = m_statusUrl.toString();
    const QString suffix = "/api/status";
    if (text.endsWith(suffix)) {
        text.chop(suffix.size());
    }
    return text;
}

void MainWindow::applyEndpoints(const QString &apiBaseUrl, const QString &wsUrl)
{
    QString normalizedApiBase = apiBaseUrl.trimmed();
    QString normalizedWsUrl = wsUrl.trimmed();

    while (normalizedApiBase.endsWith('/')) {
        normalizedApiBase.chop(1);
    }
    while (normalizedWsUrl.endsWith('/')) {
        normalizedWsUrl.chop(1);
    }

    const QUrl newStatusUrl(normalizedApiBase + "/api/status");
    const QUrl newTaskUrl(normalizedApiBase + "/api/tasks");
    const QUrl newRecentTasksUrl(normalizedApiBase + "/api/tasks/recent");
    const QUrl newRecentAlertsUrl(normalizedApiBase + "/api/alerts/recent");
    const QUrl newWsUrl(normalizedWsUrl);

    const bool apiValid = newStatusUrl.isValid() &&
                          (newStatusUrl.scheme() == "http" || newStatusUrl.scheme() == "https") &&
                          !newStatusUrl.host().isEmpty();

    const bool wsValid = newWsUrl.isValid() &&
                         (newWsUrl.scheme() == "ws" || newWsUrl.scheme() == "wss") &&
                         !newWsUrl.host().isEmpty();

    if (!apiValid || !newTaskUrl.isValid() ||
        !newRecentTasksUrl.isValid() || !newRecentAlertsUrl.isValid() ||
        !wsValid) {
        appendLog("连接配置无效，请检查 API / WS 地址格式。");
        m_settingsPage->setStatusMessage("连接配置无效，请检查 API / WS 地址格式。", false);
        return;
    }

    m_statusUrl = newStatusUrl;
    m_taskUrl = newTaskUrl;
    m_recentTasksUrl = newRecentTasksUrl;
    m_recentAlertsUrl = newRecentAlertsUrl;
    m_wsUrl = newWsUrl;

    m_settingsPage->setEndpoints(QUrl(normalizedApiBase), m_wsUrl);
    m_settingsPage->setStatusMessage("已应用连接配置，正在重连...", true);

    appendLog(QString("已应用连接配置: api=%1, ws=%2")
                  .arg(normalizedApiBase, normalizedWsUrl));

    m_client->disconnectWebSocket();

    if (!m_statusTimer->isActive()) {
        m_statusTimer->start();
    }

    refreshDashboardData();
    m_client->connectWebSocket(m_wsUrl);
}

void MainWindow::refreshDashboardData()
{
    m_client->requestCurrentStatus(m_statusUrl);
    m_client->requestRecentTasks(m_recentTasksUrl);
    m_client->requestRecentAlerts(m_recentAlertsUrl);
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
    m_client->requestRecentTasks(m_recentTasksUrl);
    appendLog("Switch to Tasks page.");
}

void MainWindow::onNavAlertsClicked()
{
    ui->stackedPages->setCurrentWidget(m_alertsPage);
    syncNavButtons(ui->btnNavAlerts);
    m_client->requestRecentAlerts(m_recentAlertsUrl);
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

    updateResizeHandlesGeometry();
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
            move(widgetPos);
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateResizeHandlesGeometry();
}