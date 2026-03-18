#include "overviewpage.h"
#include "ui_overview_page.h"

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