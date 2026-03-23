#include "taskspage.h"
#include "ui_tasks_page.h"

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>

TasksPage::TasksPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TasksPage)
{
    ui->setupUi(this);

    ui->tableRecentTasks->setColumnCount(5);
    ui->tableRecentTasks->setHorizontalHeaderLabels(
        {"任务ID", "机器人", "目标点", "状态", "时间"});
    ui->tableRecentTasks->horizontalHeader()->setStretchLastSection(true);
    ui->tableRecentTasks->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableRecentTasks->verticalHeader()->setVisible(false);
    ui->tableRecentTasks->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableRecentTasks->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->btnSubmitTask, &QPushButton::clicked,
            this, &TasksPage::onSubmitClicked);

    connect(ui->btnSoftReset, &QPushButton::clicked, this, [this]() {
        ui->textLog->clear();
        appendLog("已清空任务页日志。");
    });
}

TasksPage::~TasksPage()
{
    delete ui;
}

void TasksPage::onSubmitClicked()
{
    emit submitRequested(
        ui->editRobotId->text(),
        ui->spinTargetX->value(),
        ui->spinTargetY->value(),
        ui->comboTaskType->currentText()
    );
}

void TasksPage::appendLog(const QString &text)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textLog->appendPlainText(QString("[%1] %2").arg(ts, text));
}

QString TasksPage::statusText(const QString &status) const
{
    if (status == "accepted") return "已接收";
    if (status == "running" || status == "moving") return "执行中";
    if (status == "finished" || status == "idle") return "已完成";
    if (status == "failed") return "失败";
    if (status == "offline") return "离线";
    return status;
}

void TasksPage::updateRecentTasks(const QJsonArray &items)
{
    ui->tableRecentTasks->setRowCount(0);

    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject obj = items.at(i).toObject();
        const int row = ui->tableRecentTasks->rowCount();
        ui->tableRecentTasks->insertRow(row);

        const QString taskId = obj.value("task_id").toString();
        const QString robotId = obj.value("robot_id").toString("robot_1");
        const QString target = obj.value("target").toString(
            QString("(%1, %2)")
                .arg(obj.value("target_x").toDouble(), 0, 'f', 2)
                .arg(obj.value("target_y").toDouble(), 0, 'f', 2)
        );
        const QString status = statusText(obj.value("status").toString());
        const QString timeText = obj.value("finished_at").toString(
            obj.value("updated_at").toString(
                obj.value("created_at").toString()
            )
        );

        ui->tableRecentTasks->setItem(row, 0, new QTableWidgetItem(taskId));
        ui->tableRecentTasks->setItem(row, 1, new QTableWidgetItem(robotId));
        ui->tableRecentTasks->setItem(row, 2, new QTableWidgetItem(target));
        ui->tableRecentTasks->setItem(row, 3, new QTableWidgetItem(status));
        ui->tableRecentTasks->setItem(row, 4, new QTableWidgetItem(timeText));
    }
}