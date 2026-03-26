#include "trajectoryview.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>

TrajectoryView::TrajectoryView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);

    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing, true);
    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setInteractive(true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setBackgroundBrush(QColor("#FBFDFC"));

    QPen pathPen(QColor("#22C55E"), 2.2);
    pathPen.setCosmetic(true);
    m_pathItem = m_scene->addPath(QPainterPath(), pathPen);

    m_robotItem = m_scene->addEllipse(QRectF(-0.15, -0.15, 0.30, 0.30),
                                      QPen(Qt::NoPen),
                                      QBrush(QColor("#168C48")));

    QPen headingPen(QColor("#0F6B37"), 2.0);
    headingPen.setCosmetic(true);
    m_headingItem = m_scene->addLine(QLineF(), headingPen);

    m_originItem = m_scene->addEllipse(QRectF(-0.08, -0.08, 0.16, 0.16),
                                       QPen(QColor("#2563EB")),
                                       QBrush(QColor("#2563EB")));

    m_robotItem->hide();
    m_headingItem->hide();

    m_scene->setSceneRect(m_defaultSceneRect);
    resetViewToDefault();
}

void TrajectoryView::clearTrajectory()
{
    m_path.clear();
    m_hasPose = false;
    m_currentPose = QPointF(0.0, 0.0);
    m_currentYaw = 0.0;

    m_pathItem->setPath(QPainterPath());
    m_robotItem->hide();
    m_headingItem->hide();

    m_scene->setSceneRect(m_defaultSceneRect);
    resetViewToDefault();
}

void TrajectoryView::resetViewToDefault()
{
    resetTransform();

    // 固定比例尺：1 米 = m_basePixelsPerMeter 像素
    // Y 轴取反，让“向上”更符合地图直觉
    scale(m_basePixelsPerMeter, -m_basePixelsPerMeter);

    m_zoomLevel = 1.0;
    centerOn(0.0, 0.0);
}

void TrajectoryView::appendPointIfNeeded(const QPointF &p)
{
    if (!m_path.isEmpty()) {
        const QPointF last = m_path.constLast();
        const double dx = last.x() - p.x();
        const double dy = last.y() - p.y();
        if ((dx * dx + dy * dy) < 0.0004) {
            return;
        }
    }

    m_path.push_back(p);

    if (m_path.size() > 2000) {
        m_path.remove(0, m_path.size() - 2000);
    }
}

void TrajectoryView::rebuildPathItem()
{
    QPainterPath path;
    if (!m_path.isEmpty()) {
        path.moveTo(m_path.first());
        for (int i = 1; i < m_path.size(); ++i) {
            path.lineTo(m_path.at(i));
        }
    }
    m_pathItem->setPath(path);
}

void TrajectoryView::updateRobotItems(bool online)
{
    if (!m_hasPose) {
        m_robotItem->hide();
        m_headingItem->hide();
        return;
    }

    m_robotItem->show();
    m_headingItem->show();

    const QColor robotColor = online ? QColor("#168C48") : QColor("#DC2626");
    m_robotItem->setBrush(QBrush(robotColor));
    m_robotItem->setRect(QRectF(m_currentPose.x() - 0.15,
                                m_currentPose.y() - 0.15,
                                0.30,
                                0.30));

    const double headingLen = 0.55;
    const QPointF head(
        m_currentPose.x() + headingLen * std::cos(m_currentYaw),
        m_currentPose.y() + headingLen * std::sin(m_currentYaw));

    m_headingItem->setLine(QLineF(m_currentPose, head));
}

void TrajectoryView::ensureSceneContains(const QPointF &p)
{
    QRectF rect = m_scene->sceneRect();

    const double margin = 1.5;
    bool changed = false;

    if (p.x() < rect.left() + margin) {
        rect.setLeft(p.x() - 5.0);
        changed = true;
    }
    if (p.x() > rect.right() - margin) {
        rect.setRight(p.x() + 5.0);
        changed = true;
    }
    if (p.y() < rect.top() + margin) {
        rect.setTop(p.y() - 5.0);
        changed = true;
    }
    if (p.y() > rect.bottom() - margin) {
        rect.setBottom(p.y() + 5.0);
        changed = true;
    }

    if (changed) {
        m_scene->setSceneRect(rect.normalized());
    }
}

void TrajectoryView::applyZoom(double factor)
{
    const double nextZoom = m_zoomLevel * factor;
    if (nextZoom < m_minZoom || nextZoom > m_maxZoom) {
        return;
    }

    scale(factor, factor);
    m_zoomLevel = nextZoom;
}

void TrajectoryView::updateRobotPose(double x, double y, double yaw, bool online)
{
    const QPointF p(x, y);

    m_currentPose = p;
    m_currentYaw = yaw;
    m_hasPose = true;

    appendPointIfNeeded(p);
    rebuildPathItem();
    updateRobotItems(online);
    ensureSceneContains(p);

    // 默认视角下，轻微跟随机器人；
    // 用户手动缩放后，不打扰用户视图
    if (std::abs(m_zoomLevel - 1.0) < 1e-6) {
        centerOn(0.0, 0.0);
    }

    setToolTip(QString("当前位置: x=%1, y=%2, yaw=%3")
                   .arg(x, 0, 'f', 2)
                   .arg(y, 0, 'f', 2)
                   .arg(yaw, 0, 'f', 2));
}

void TrajectoryView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor("#FBFDFC"));

    // 细网格
    QPen gridPen(QColor("#E6EEE8"));
    gridPen.setCosmetic(true);
    painter->setPen(gridPen);

    const double grid = 1.0;

    const double left = std::floor(rect.left() / grid) * grid;
    const double right = std::ceil(rect.right() / grid) * grid;
    const double top = std::floor(rect.top() / grid) * grid;
    const double bottom = std::ceil(rect.bottom() / grid) * grid;

    for (double x = left; x <= right; x += grid) {
        painter->drawLine(QLineF(x, top, x, bottom));
    }
    for (double y = top; y <= bottom; y += grid) {
        painter->drawLine(QLineF(left, y, right, y));
    }

    // 坐标轴
    QPen axisPen(QColor("#BFD8C6"));
    axisPen.setCosmetic(true);
    axisPen.setWidth(2);
    painter->setPen(axisPen);
    painter->drawLine(QLineF(0.0, top, 0.0, bottom));
    painter->drawLine(QLineF(left, 0.0, right, 0.0));

    // 刻度线
    QPen tickPen(QColor("#8FA99A"));
    tickPen.setCosmetic(true);
    tickPen.setWidth(1);
    painter->setPen(tickPen);

    const double tickHalfLen = 0.12;
    for (double x = left; x <= right; x += grid) {
        painter->drawLine(QLineF(x, -tickHalfLen, x, tickHalfLen));
    }
    for (double y = top; y <= bottom; y += grid) {
        painter->drawLine(QLineF(-tickHalfLen, y, tickHalfLen, y));
    }

    // 刻度文字
    painter->setPen(QColor("#6B7D71"));
    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);

    for (double x = left; x <= right; x += grid) {
        if (qFuzzyIsNull(x)) {
            continue;
        }
        painter->drawText(QPointF(x + 0.08, 0.35), QString::number(x, 'f', 0));
    }

    for (double y = top; y <= bottom; y += grid) {
        if (qFuzzyIsNull(y)) {
            continue;
        }
        painter->drawText(QPointF(0.15, y - 0.08), QString::number(y, 'f', 0));
    }

    // 原点标签
    painter->setPen(QColor("#2563EB"));
    painter->drawText(QPointF(0.15, -0.15), QStringLiteral("O"));

    if (!m_hasPose && m_path.isEmpty()) {
        painter->setPen(QColor("#7A8B7E"));
        painter->drawText(rect, Qt::AlignCenter, QStringLiteral("等待轨迹数据..."));
    }
}

void TrajectoryView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    // 窗口尺寸变化时，只在默认缩放下重新居中，
    // 不再按 sceneRect 重新 fit，避免轨迹缩成一团
    if (std::abs(m_zoomLevel - 1.0) < 1e-6) {
        centerOn(0.0, 0.0);
    }
}

void TrajectoryView::wheelEvent(QWheelEvent *event)
{
    const QPoint delta = event->angleDelta();
    if (delta.y() == 0) {
        event->ignore();
        return;
    }

    const double factor = (delta.y() > 0) ? 1.15 : (1.0 / 1.15);
    applyZoom(factor);
    event->accept();
}

void TrajectoryView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resetViewToDefault();
        event->accept();
        return;
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}