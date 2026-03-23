#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QJsonObject>
#include <QPointF>
#include <QVector>
#include <QWidget>

namespace Ui {
class OverviewPage;
}

class QResizeEvent;

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);
    ~OverviewPage();

    void updateStatus(const QJsonObject &status);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void appendPathPoint(double x, double y);
    void redrawTrajectory(double x, double y, double yaw);

private:
    Ui::OverviewPage *ui;
    QVector<QPointF> m_path;
};

#endif // OVERVIEWPAGE_H