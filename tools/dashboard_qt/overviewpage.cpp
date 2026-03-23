#include "overviewpage.h"
#include "ui_overview_page.h"

#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QResizeEvent>

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OverviewPage)
{
    ui->setupUi(this);
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::appendPathPoint(double x, double y)
{
    const QPointF p(x, y);
    if (!m_path.isEmpty()) {
        const QPointF last = m_path.constLast();
        const double dx = last.x() - p.x();
        const double dy = last.y() - p.y();
        if ((dx * dx + dy * dy) < 0.0004) {
            return;
        }
    }

    m_path.push_back(p);
    if (m_path.size() > 200) {
        m_path.remove(0);
    }
}

void OverviewPage::redrawTrajectory(double x, double y, double yaw)
{
    const int w = ui->labelMapHint->width();
    const int h = ui->labelMapHint->height();
    if (w <= 10 || h <= 10) {
        return;
    }

    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect(0, 0, w - 1, h - 1);
    painter.setPen(QPen(QColor("#CFE5D5"), 1, Qt::DashLine));
    painter.setBrush(QColor("#FBFDFC"));
    painter.drawRoundedRect(rect, 14, 14);

    if (m_path.isEmpty()) {
        painter.setPen(QColor("#7A8B7E"));
        painter.drawText(rect, Qt::AlignCenter, "等待轨迹数据...");
        ui->labelMapHint->setPixmap(pixmap);
        return;
    }

    double minX = m_path.first().x();
    double maxX = m_path.first().x();
    double minY = m_path.first().y();
    double maxY = m_path.first().y();

    for (const QPointF &p : std::as_const(m_path)) {
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }

    minX = qMin(minX, x);
    maxX = qMax(maxX, x);
    minY = qMin(minY, y);
    maxY = qMax(maxY, y);

    const double padWorld = 0.5;
    minX -= padWorld;
    maxX += padWorld;
    minY -= padWorld;
    maxY += padWorld;

    const double spanX = qMax(1.0, maxX - minX);
    const double spanY = qMax(1.0, maxY - minY);
    const double padPx = 24.0;
    const double scale = qMin((w - 2.0 * padPx) / spanX,
                              (h - 2.0 * padPx) / spanY);

    auto mapPoint = [&](double px, double py) {
        const double sx = padPx + (px - minX) * scale;
        const double sy = h - padPx - (py - minY) * scale;
        return QPointF(sx, sy);
    };

    painter.setPen(QPen(QColor("#E6EEE8"), 1));
    for (int i = 1; i <= 4; ++i) {
        const double gx = padPx + i * (w - 2.0 * padPx) / 5.0;
        const double gy = padPx + i * (h - 2.0 * padPx) / 5.0;
        painter.drawLine(QPointF(gx, padPx), QPointF(gx, h - padPx));
        painter.drawLine(QPointF(padPx, gy), QPointF(w - padPx, gy));
    }

    QPolygonF polyline;
    for (const QPointF &p : std::as_const(m_path)) {
        polyline << mapPoint(p.x(), p.y());
    }

    painter.setPen(QPen(QColor("#22C55E"), 3));
    painter.drawPolyline(polyline);

    const QPointF current = mapPoint(x, y);
    painter.setBrush(QColor("#168C48"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(current, 6, 6);

    painter.setPen(QPen(QColor("#0F6B37"), 2));
    const double headingLen = 20.0;
    const QPointF head(
        current.x() + headingLen * std::cos(yaw),
        current.y() - headingLen * std::sin(yaw));
    painter.drawLine(current, head);

    ui->labelMapHint->setPixmap(pixmap);
    ui->labelMapHint->setToolTip(
        QString("当前位置: x=%1, y=%2, yaw=%3")
            .arg(x, 0, 'f', 2)
            .arg(y, 0, 'f', 2)
            .arg(yaw, 0, 'f', 2));
}

void OverviewPage::updateStatus(const QJsonObject &status)
{
    const QString robotId = status.value("robot_id").toString("robot_1");
    const double x = status.value("x").toDouble();
    const double y = status.value("y").toDouble();
    const double yaw = status.value("yaw").toDouble();
    const double battery = status.value("battery_pct").toDouble();
    const QString mode = status.value("mode").toString("unknown");
    const QString taskId = status.value("task_id").toString();
    const QString alert = status.value("alert_text").toString();
    const bool online = status.value("is_online").toBool();

    ui->labelRobotIdValue->setText(robotId);
    ui->linePose->setText(
        QString("x=%1, y=%2, yaw=%3")
            .arg(x, 0, 'f', 2)
            .arg(y, 0, 'f', 2)
            .arg(yaw, 0, 'f', 2));

    ui->lineMode->setText(mode);
    ui->plainAlertText->setPlainText(alert.isEmpty() ? "无告警" : alert);

    ui->statValue->setText(online ? "在线" : "离线");
    ui->statValue->setStyleSheet(
        online
            ? "color:#168C48; font:700 18pt \"Microsoft YaHei\";"
            : "color:#DC2626; font:700 18pt \"Microsoft YaHei\";");

    ui->statValue2->setText(QString("%1%").arg(QString::number(battery, 'f', 1)));
    ui->statValue3->setText(taskId.isEmpty() ? "-" : taskId);
    ui->statValue4->setText(alert.isEmpty() ? "0" : "1");

    appendPathPoint(x, y);
    redrawTrajectory(x, y, yaw);
}

void OverviewPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!m_path.isEmpty()) {
        const QPointF p = m_path.constLast();
        redrawTrajectory(p.x(), p.y(), 0.0);
    }
}