#ifndef DATASETBROWSER_H
#define DATASETBROWSER_H

#include "dialog.h"
#include "ui_datasetbrowser.h"

class DatasetBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit DatasetBrowser(const std::vector<QStringList>& datasetContainer, QWidget *parent = nullptr);
    ~DatasetBrowser();
    void connectToUi(Dialog *uiDialog);
private slots:
    void selectButton();
private:
    Ui::DatasetBrowser *ui;
    Dialog *baseUi;
    DatasetModel* datasets_m = new DatasetModel();
};



#endif // DATASETBROWSER_H
