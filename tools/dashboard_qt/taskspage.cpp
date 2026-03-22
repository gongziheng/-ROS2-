#include "taskspage.h"
#include "ui_tasks_page.h"

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

TasksPage::TasksPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TasksPage)
{
    ui->setupUi(this);

    if (auto *btn = findChild<QPushButton *>("btnSubmitTask")) {
        connect(btn, &QPushButton::clicked, this, &TasksPage::onSubmitClicked);
    }
}

TasksPage::~TasksPage()
{
    delete ui;
}

void TasksPage::onSubmitClicked()
{
    const auto *robotEdit = findChild<QLineEdit *>("editRobotId");
    const auto *xSpin = findChild<QDoubleSpinBox *>("spinTargetX");
    const auto *ySpin = findChild<QDoubleSpinBox *>("spinTargetY");
    const auto *taskCombo = findChild<QComboBox *>("comboTaskType");

    if (!robotEdit || !xSpin || !ySpin || !taskCombo) {
        appendLog("任务页控件未找到，请检查 objectName。");
        return;
    }

    emit submitRequested(
        robotEdit->text(),
        xSpin->value(),
        ySpin->value(),
        taskCombo->currentText()
    );
}

void TasksPage::appendLog(const QString &text)
{
    if (auto *plain = findChild<QPlainTextEdit *>("textLog")) {
        const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        plain->appendPlainText(QString("[%1] %2").arg(ts, text));
    }
}