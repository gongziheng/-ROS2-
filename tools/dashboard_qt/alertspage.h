#ifndef ALERTSPAGE_H
#define ALERTSPAGE_H

#include <QWidget>

namespace Ui {
class AlertsPage;
}

class AlertsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AlertsPage(QWidget *parent = nullptr);
    ~AlertsPage();

private:
    Ui::AlertsPage *ui;
};

#endif // ALERTSPAGE_H