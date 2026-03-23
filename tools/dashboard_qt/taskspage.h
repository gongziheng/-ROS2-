#ifndef TASKSPAGE_H
#define TASKSPAGE_H

#include <QJsonArray>
#include <QWidget>

namespace Ui {
class TasksPage;
}

class TasksPage : public QWidget
{
    Q_OBJECT

public:
    explicit TasksPage(QWidget *parent = nullptr);
    ~TasksPage();

    void appendLog(const QString &text);
    void updateRecentTasks(const QJsonArray &items);

signals:
    void submitRequested(const QString &robotId,
                         double targetX,
                         double targetY,
                         const QString &taskType);

private:
    void onSubmitClicked();
    QString statusText(const QString &status) const;

private:
    Ui::TasksPage *ui;
};

#endif // TASKSPAGE_H