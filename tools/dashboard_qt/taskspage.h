#ifndef TASKSPAGE_H
#define TASKSPAGE_H

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

signals:
    void submitRequested(const QString &robotId, double targetX,
                         double targetY, const QString &taskType);
                        
private:
    void onSubmitClicked();

private:
    Ui::TasksPage *ui;
};

#endif // TASKSPAGE_H