#include "datasetbrowser.h"
#include "ui_datasetbrowser.h"

DatasetBrowser::DatasetBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DatasetBrowser)
{
    ui->setupUi(this);
}

void DatasetBrowser::connectToUi(Ui::Dialog *ui){
    baseUi = ui;
}

DatasetBrowser::~DatasetBrowser()
{
    delete ui;
}
