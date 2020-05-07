#include "logbrowser.h"


LogBrowser::LogBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogBrowser)
{
    ui->setupUi(this);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(close()));
}

LogBrowser::~LogBrowser()
{
    delete ui;
}
