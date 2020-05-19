#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include "dialog.h"
#include "ui_filebrowser.h"

class fileBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit fileBrowser(QWidget *parent = nullptr);
    ~fileBrowser();
    bool removeDuplicates = true;

private slots:
    void getCurrentEntry();
    void setParentDirActive();
    void selectButton();



private:
    Ui::fileBrowser *uiFileBrowser;
    QFileSystemModel *fileModel;
    QStringList filePathQList;
    std::string filePath;
    Dialog *baseUi;

    void appendEntry();

};

#endif // FILEBROWSER_H
