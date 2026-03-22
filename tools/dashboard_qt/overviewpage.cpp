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

void OverviewPage::setLabelIfExists(const char *name, const QString &text)
{
    if (auto *label = findChild<QLabel *>(name)) {
        label->setText(text);
    }
}

void OverviewPage::updateStatus(const QJsonObject &status)
{
    const QString robotId = status.value("robot_id").toString();
    const double x = status.value("x").toDouble();
    const double y = status.value("y").toDouble();
    const double yaw = status.value("yaw").toDouble();
    const double battery = status.value("battery_pct").toDouble();
    const QString mode = status.value("mode").toString();
    const QString taskId = status.value("task_id").toString();
    const QString alertText = status.value("alert_text").toString();
    const bool isOnline = status.value("is_online").toBool();

    // 机器人基础信息
    setLabelIfExists("labelRobotIdValue", robotId);
    setLabelIfExists("labelOnlineValue", isOnline ? "在线" : "离线");
    setLabelIfExists("labelBatteryValue", QString::number(battery, 'f', 1) + "%");
    setLabelIfExists("labelTaskIdValue", taskId.isEmpty() ? "-" : taskId);
    setLabelIfExists("labelModeValue", mode.isEmpty() ? "-" : mode);

    // 位姿
    if (auto *line = findChild<QLineEdit *>("linePose")) {
        line->setText(QString("x=%1, y=%2, yaw=%3")
                          .arg(x, 0, 'f', 2)
                          .arg(y, 0, 'f', 2)
                          .arg(yaw, 0, 'f', 2));
    }

    // 模式行：保留概览摘要，但不要当唯一显示
    if (auto *line = findChild<QLineEdit *>("lineMode")) {
        line->setText(QString("%1 | 电量 %2% | 任务 %3")
                          .arg(mode.isEmpty() ? "-" : mode)
                          .arg(QString::number(battery, 'f', 1))
                          .arg(taskId.isEmpty() ? "-" : taskId));
    }

    // 告警文本
    if (auto *plain = findChild<QPlainTextEdit *>("plainAlertText")) {
        plain->setPlainText(alertText.isEmpty() ? "无告警" : alertText);
    }

    // 告警数：当前接口没有单独字段，先按“有告警文本=1，无告警=0”
    setLabelIfExists("labelAlertCountValue", alertText.isEmpty() ? "0" : "1");
}