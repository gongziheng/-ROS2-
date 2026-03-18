#include "settingspage.h"
#include "ui_settings_page.h"

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsPage)
{
    ui->setupUi(this);
}

SettingsPage::~SettingsPage()
{
    delete ui;
}