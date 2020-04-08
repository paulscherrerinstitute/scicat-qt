#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QDialog>
#include <QFileSystemModel>
#include "dialog.h"

namespace Ui {
class fileBrowser;
class Dialog;
}

class fileBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit fileBrowser(QWidget *parent = nullptr);
    ~fileBrowser();
    void connectToUi(Ui::Dialog *ui);

private slots:
    void getCurrentEntry();
    void setParentDirActive();
    void setChildDirActive();
    void selectButton();


private:
    Ui::fileBrowser *uiFileBrowser;
    QFileSystemModel *fileModel;
    std::string filePath;
    Ui::Dialog *baseUi;


};

#endif // FILEBROWSER_H
