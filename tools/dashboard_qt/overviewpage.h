#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QJsonObject>
#include <QWidget>

namespace Ui {
class OverviewPage;
}

class QResizeEvent;
class TrajectoryView;

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
    Ui::OverviewPage *ui;
    TrajectoryView *m_trajView = nullptr;
    double m_lastYaw = 0.0;
};

#endif // OVERVIEWPAGE_H