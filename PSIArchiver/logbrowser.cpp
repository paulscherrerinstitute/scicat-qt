#include "logbrowser.h"


LogBrowser::LogBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogBrowser)
{
    ui->setupUi(this);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pushButtonSaveLog, SIGNAL(clicked()), this, SLOT(saveLog()));

}

void LogBrowser::saveLog(){
    auto fileName = QFileDialog::getSaveFileName(this, tr("Save File"),  QDir::homePath(), tr("Log files (*.log)"));
}

LogBrowser::~LogBrowser()
{
    delete ui;
}
