#include "datasetbrowser.h"



DatasetBrowser::DatasetBrowser(const std::vector<QStringList>& datasetContainer, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DatasetBrowser)
{
    ui->setupUi(this);
    datasets_m->setContainer(datasetContainer);
    ui->tableViewDatasetBrowser->setModel(datasets_m);
    ui->tableViewDatasetBrowser->setColumnWidth(0, 200);
    ui->tableViewDatasetBrowser->setColumnWidth(1, 200);
    ui->tableViewDatasetBrowser->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    ui->tableViewDatasetBrowser->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->pushButton, SIGNAL (clicked()), this, SLOT (selectButton()));

    // connect to parent
    baseUi = dynamic_cast<Dialog*>(parent);
}

void DatasetBrowser::selectButton(){
    QModelIndexList indexList = ui->tableViewDatasetBrowser->selectionModel()->selectedRows();
    for (int ii=0; ii<indexList.size(); ++ii) {
        QStringList tmp;
        baseUi->ui->tableWidget->insertRow(0);
        for (int colIndx=0; colIndx<datasets_m->columnCount(); colIndx++) {
            QTableWidgetItem *tmpItem = new QTableWidgetItem(datasets_m->data(indexList[ii].siblingAtColumn(colIndx),0).toString());
            baseUi->ui->tableWidget->setItem(0,colIndx,tmpItem);
        }
        baseUi->ui->clearAllButtonDatasets->setEnabled(true);
        baseUi->ui->clearButtonDatasets->setEnabled(true);

    }
    close();

}

DatasetBrowser::~DatasetBrowser()
{
    delete datasets_m;
    delete ui;
}
