#include "alertspage.h"
#include "ui_alerts_page.h"

AlertsPage::AlertsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlertsPage)
{
    ui->setupUi(this);
}

AlertsPage::~AlertsPage()
{
    delete ui;
}