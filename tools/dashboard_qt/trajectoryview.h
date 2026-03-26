#ifndef TRAJECTORYVIEW_H
#define TRAJECTORYVIEW_H

#include <QGraphicsView>
#include <QPointF>
#include <QVector>

class QGraphicsScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QWheelEvent;
class QMouseEvent;
class QResizeEvent;
class QPainter;

class TrajectoryView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit TrajectoryView(QWidget *parent = nullptr);

    void updateRobotPose(double x, double y, double yaw, bool online);
    void clearTrajectory();
    void resetViewToDefault();

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void appendPointIfNeeded(const QPointF &p);
    void rebuildPathItem();
    void updateRobotItems(bool online);
    void ensureSceneContains(const QPointF &p);
    void applyZoom(double factor);

    void rebuildViewTransform(const QPointF &center);
    double defaultPixelsPerMeter() const;

private:
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPathItem *m_pathItem = nullptr;
    QGraphicsEllipseItem *m_robotItem = nullptr;
    QGraphicsLineItem *m_headingItem = nullptr;
    QGraphicsEllipseItem *m_originItem = nullptr;

    QVector<QPointF> m_path;
    QPointF m_currentPose {0.0, 0.0};
    double m_currentYaw = 0.0;
    bool m_hasPose = false;

    QRectF m_defaultSceneRect {-6.0, -6.0, 12.0, 12.0};

    double m_zoomLevel = 1.0;
    const double m_minZoom = 0.5;
    const double m_maxZoom = 8.0;

    // 默认希望看到约 8 米高的世界区域
    const double m_defaultVisibleHeightMeters = 8.0;
};

#endif // TRAJECTORYVIEW_H