#include "alertspage.h"
#include "ui_alerts_page.h"

#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>

AlertsPage::AlertsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlertsPage)
{
    ui->setupUi(this);

    ui->tableRecentAlerts->setColumnCount(5);
    ui->tableRecentAlerts->setHorizontalHeaderLabels(
        {"时间", "机器人", "级别", "类型", "说明"});
    ui->tableRecentAlerts->horizontalHeader()->setStretchLastSection(true);
    ui->tableRecentAlerts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableRecentAlerts->verticalHeader()->setVisible(false);
    ui->tableRecentAlerts->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableRecentAlerts->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

AlertsPage::~AlertsPage()
{
    delete ui;
}

QString AlertsPage::levelText(const QString &level) const
{
    if (level == "critical") return "严重";
    if (level == "warning") return "警告";
    if (level == "info") return "提示";
    return level;
}

QString AlertsPage::codeText(const QString &code) const
{
    if (code == "offline") return "离线";
    if (code == "low_battery") return "低电量";
    if (code == "blocked") return "阻塞";
    if (code == "robot_alert") return "机器人告警";
    return code;
}

void AlertsPage::updateRecentAlerts(const QJsonArray &items)
{
    ui->tableRecentAlerts->setRowCount(0);

    int activeCount = 0;
    int recoveredCount = 0;
    const int totalCount = items.size();

    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject obj = items.at(i).toObject();
        const int row = ui->tableRecentAlerts->rowCount();
        ui->tableRecentAlerts->insertRow(row);

        const QString timeText = obj.value("time").toString();
        const QString robotId = obj.value("robot_id").toString("robot_1");
        const QString level = obj.value("level").toString();
        const QString code = obj.value("code").toString();
        const QString message = obj.value("message").toString();
        const bool active = obj.value("active").toBool(true);

        if (active) {
            ++activeCount;
        } else {
            ++recoveredCount;
        }

        auto *timeItem = new QTableWidgetItem(timeText);
        auto *robotItem = new QTableWidgetItem(robotId);
        auto *levelItem = new QTableWidgetItem(levelText(level));
        auto *codeItem = new QTableWidgetItem(codeText(code));
        auto *msgItem = new QTableWidgetItem(
            active ? message : QString("%1（已恢复）").arg(message));

        if (level == "critical") {
            levelItem->setForeground(QBrush(QColor("#DC2626")));
        } else if (level == "warning") {
            levelItem->setForeground(QBrush(QColor("#D97706")));
        } else {
            levelItem->setForeground(QBrush(QColor("#168C48")));
        }

        ui->tableRecentAlerts->setItem(row, 0, timeItem);
        ui->tableRecentAlerts->setItem(row, 1, robotItem);
        ui->tableRecentAlerts->setItem(row, 2, levelItem);
        ui->tableRecentAlerts->setItem(row, 3, codeItem);
        ui->tableRecentAlerts->setItem(row, 4, msgItem);
    }

    ui->statValue->setText(QString::number(activeCount));
    ui->statValue2->setText(QString::number(totalCount));
    ui->statValue3->setText(QString::number(recoveredCount));
}