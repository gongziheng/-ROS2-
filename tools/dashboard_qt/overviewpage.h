#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QJsonObject>
#include <QWidget>

namespace Ui {
class OverviewPage;
}

class TrajectoryView;

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);
    ~OverviewPage();

    void updateStatus(const QJsonObject &status);

private:
    Ui::OverviewPage *ui = nullptr;
    TrajectoryView *m_trajView = nullptr;
    double m_lastYaw = 0.0;
};

#endif // OVERVIEWPAGE_H