#ifndef RESIZEHANDLE_H
#define RESIZEHANDLE_H

#include <QWidget>
#include <QPoint>
#include <QRect>

class ResizeHandle : public QWidget
{
    Q_OBJECT

public:
    enum Region {
        Left,
        Top,
        Right,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    explicit ResizeHandle(QWidget *targetWindow, Region region, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateCursor();
    void resizeTarget(const QPoint &globalPos);

private:
    QWidget *m_targetWindow = nullptr;
    Region m_region;

    bool m_pressed = false;
    QPoint m_pressGlobalPos;
    QRect m_pressGeometry;
};

#endif // RESIZEHANDLE_H