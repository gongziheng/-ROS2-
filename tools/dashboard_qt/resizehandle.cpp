#include "resizehandle.h"

#include <QMouseEvent>

ResizeHandle::ResizeHandle(QWidget *targetWindow, Region region, QWidget *parent)
    : QWidget(parent)
    , m_targetWindow(targetWindow)
    , m_region(region)
{
    setMouseTracking(true);
    setStyleSheet("background: transparent;");
    updateCursor();
}

void ResizeHandle::updateCursor()
{
    switch (m_region) {
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    }
}

void ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    if (!m_targetWindow || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_targetWindow->isMaximized()) {
        QWidget::mousePressEvent(event);
        return;
    }

    m_pressed = true;
    m_pressGlobalPos = event->globalPos();
    m_pressGeometry = m_targetWindow->geometry();

    grabMouse();
    QWidget::mousePressEvent(event);
}

void ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed && m_targetWindow) {
        resizeTarget(event->globalPos());
    }

    QWidget::mouseMoveEvent(event);
}

void ResizeHandle::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressed) {
        m_pressed = false;
        releaseMouse();
    }

    QWidget::mouseReleaseEvent(event);
}

void ResizeHandle::resizeTarget(const QPoint &globalPos)
{
    const QRect g = m_pressGeometry;
    const QPoint delta = globalPos - m_pressGlobalPos;

    const int minW = m_targetWindow->minimumWidth();
    const int minH = m_targetWindow->minimumHeight();

    int left = g.left();
    int top = g.top();
    int right = g.right();
    int bottom = g.bottom();

    switch (m_region) {
    case Left:
        left = qMin(g.left() + delta.x(), right - minW + 1);
        break;
    case Top:
        top = qMin(g.top() + delta.y(), bottom - minH + 1);
        break;
    case Right:
        right = qMax(g.right() + delta.x(), left + minW - 1);
        break;
    case Bottom:
        bottom = qMax(g.bottom() + delta.y(), top + minH - 1);
        break;
    case TopLeft:
        left = qMin(g.left() + delta.x(), right - minW + 1);
        top = qMin(g.top() + delta.y(), bottom - minH + 1);
        break;
    case TopRight:
        right = qMax(g.right() + delta.x(), left + minW - 1);
        top = qMin(g.top() + delta.y(), bottom - minH + 1);
        break;
    case BottomLeft:
        left = qMin(g.left() + delta.x(), right - minW + 1);
        bottom = qMax(g.bottom() + delta.y(), top + minH - 1);
        break;
    case BottomRight:
        right = qMax(g.right() + delta.x(), left + minW - 1);
        bottom = qMax(g.bottom() + delta.y(), top + minH - 1);
        break;
    }

    m_targetWindow->setGeometry(QRect(QPoint(left, top), QPoint(right, bottom)));
}