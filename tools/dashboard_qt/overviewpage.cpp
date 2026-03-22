#include "overviewpage.h"
#include "ui_overview_page.h"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OverviewPage)
{
    ui->setupUi(this);
}

void OverviewPage::updateStatus(const QJsonObject &status)
{
    if (auto *label = findChild<QLabel *>("labelRobotIdValue")) {
        label->setText(status.value("robot_id").toString());
    }

    if (auto *line = findChild<QLineEdit *>("linePose")) {
        const double x = status.value("x").toDouble();
        const double y = status.value("y").toDouble();
        const double yaw = status.value("yaw").toDouble();
        line->setText(QString("x=%1, y=%2, yaw=%3")
                          .arg(x, 0, 'f', 2)
                          .arg(y, 0, 'f', 2)
                          .arg(yaw, 0, 'f', 2));
    }

    if (auto *line = findChild<QLineEdit *>("lineMode")) {
        const QString mode = status.value("mode").toString();
        const double battery = status.value("battery_pct").toDouble();
        const QString taskId = status.value("task_id").toString();
        line->setText(QString("%1 | 电量 %2% | 任务 %3")
                          .arg(mode)
                          .arg(QString::number(battery, 'f', 1))
                          .arg(taskId));
    }

    if (auto *plain = findChild<QPlainTextEdit *>("plainAlertText")) {
        const QString alert = status.value("alert_text").toString();
        plain->setPlainText(alert.isEmpty() ? "无告警" : alert);
    }
}

OverviewPage::~OverviewPage()
{
    delete ui;
}