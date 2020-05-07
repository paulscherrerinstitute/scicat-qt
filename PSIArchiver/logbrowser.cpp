#include "logbrowser.h"
#include "ui_logbrowser.h"
#include "dialog.h"

LogBrowser::LogBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogBrowser)
{
    ui->setupUi(this);
}

LogBrowser::~LogBrowser()
{
    delete ui;
}
