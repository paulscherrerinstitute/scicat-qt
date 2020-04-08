#ifndef DATASETBROWSER_H
#define DATASETBROWSER_H

#include <QDialog>

namespace Ui {
class DatasetBrowser;
}

class DatasetBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit DatasetBrowser(QWidget *parent = nullptr);
    ~DatasetBrowser();

private:
    Ui::DatasetBrowser *ui;
};

#endif // DATASETBROWSER_H
