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

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::updateStatus(const QJsonObject &status)
{
    const QString robotId = status.value("robot_id").toString("robot_1");
    const double x = status.value("x").toDouble();
    const double y = status.value("y").toDouble();
    const double yaw = status.value("yaw").toDouble();
    const double battery = status.value("battery_pct").toDouble();
    const QString mode = status.value("mode").toString("unknown");
    const QString taskId = status.value("task_id").toString();
    const QString alert = status.value("alert_text").toString();
    const bool online = status.value("is_online").toBool();

    ui->labelRobotIdValue->setText(robotId);
    ui->linePose->setText(
        QString("x=%1, y=%2, yaw=%3")
            .arg(x, 0, 'f', 2)
            .arg(y, 0, 'f', 2)
            .arg(yaw, 0, 'f', 2));

    ui->lineMode->setText(mode);
    ui->plainAlertText->setPlainText(alert.isEmpty() ? "无告警" : alert);

    // 顶部 4 张统计卡
    ui->statValue->setText(online ? "在线" : "离线");
    ui->statValue->setStyleSheet(
        online
            ? "color:#168C48; font:700 18pt \"Microsoft YaHei\";"
            : "color:#DC2626; font:700 18pt \"Microsoft YaHei\";");

    ui->statValue2->setText(QString("%1%")
                                .arg(QString::number(battery, 'f', 1)));

    ui->statValue3->setText(taskId.isEmpty() ? "-" : taskId);

    ui->statValue4->setText(alert.isEmpty() ? "0" : "1");

    // 地图区先给个可见反馈
    ui->labelMapHint->setText(
        QString("当前位置: x=%1, y=%2, yaw=%3")
            .arg(x, 0, 'f', 2)
            .arg(y, 0, 'f', 2)
            .arg(yaw, 0, 'f', 2));
}