#include "taskspage.h"
#include "ui_tasks_page.h"

TasksPage::TasksPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TasksPage)
{
    ui->setupUi(this);
}

TasksPage::~TasksPage()
{
    delete ui;
}