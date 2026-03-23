#ifndef ALERTSPAGE_H
#define ALERTSPAGE_H

#include <QJsonArray>
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

    void updateRecentAlerts(const QJsonArray &items);

private:
    QString levelText(const QString &level) const;
    QString codeText(const QString &code) const;

private:
    Ui::AlertsPage *ui;
};

#endif // ALERTSPAGE_H