#include "filebrowser.h"
#include "ui_filebrowser.h"
#include <iostream>
#include <QMainWindow>

fileBrowser::fileBrowser(QWidget *parent) :
    QDialog(parent),
    uiFileBrowser(new Ui::fileBrowser)
{
    uiFileBrowser->setupUi(this);

    QString sPath = "/";
    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(sPath);
    uiFileBrowser->columnView->setModel(fileModel);
    uiFileBrowser->columnView->setRootIndex(fileModel->index(sPath));
//    uiFileBrowser->columnView->show();

    connect(uiFileBrowser->pushButtonFileBrowser, SIGNAL (clicked()), this, SLOT (selectButton()));
    connect(uiFileBrowser->columnView->selectionModel(), SIGNAL (doubleClicked()), this, SLOT (setChildDirActive()));
    connect(uiFileBrowser->toolButtonUp, SIGNAL (clicked()), this, SLOT (setParentDirActive()));

}

void fileBrowser::connectToUi(Ui::Dialog *ui){
    baseUi = ui;
}
void fileBrowser::selectButton(){
    getCurrentEntry();

    std::cout << baseUi->filePath << std::endl;

    close();
}

void fileBrowser::getCurrentEntry(){
    QModelIndex index = uiFileBrowser->columnView->currentIndex();
    QVariant data = uiFileBrowser->columnView->model()->data(index);
    QString file =  data.toString();
    filePath = "/" + file.toStdString();
    QModelIndex parentIndex, parentIndexOld;
    QVariant dataParent;
    QString dataParentString;
    parentIndexOld = index;
    std::string parentStdString;
    while (true){
        parentIndex = uiFileBrowser->columnView->model()->parent(parentIndexOld);
        if (!parentIndex.isValid()){
            break;
        }
        dataParent = uiFileBrowser->columnView->model()->data(parentIndex);
        dataParentString = dataParent.toString();
        parentStdString = dataParentString.toStdString();
        if (parentStdString.compare("/")==0){
            break;
        }
        filePath.insert(0,"/" + parentStdString);
        parentIndexOld = parentIndex;
    }
    std::cout << filePath << std::endl;

}

void fileBrowser::setParentDirActive(){
    std::cout << "Parent" << std::endl;

}


void fileBrowser::setChildDirActive(){
    std::cout << "Child" << std::endl;

}

fileBrowser::~fileBrowser()
{
    delete uiFileBrowser;
    delete fileModel;
}

