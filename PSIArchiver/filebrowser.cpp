#include "filebrowser.h"

fileBrowser::fileBrowser(QWidget *parent) :
    QDialog(parent),
    uiFileBrowser(new Ui::fileBrowser)
{
    uiFileBrowser->setupUi(this);
    baseUi = dynamic_cast<Dialog*>(parent);
    QSettings settingsContainer("PSI", "SciCat");
    QDir cwd;
    QDir pgroupPath = QDir("/das/work/"+QString::fromStdString(baseUi->metadataContainer.pgroup).midRef(0,4)+"/"+QString::fromStdString(baseUi->metadataContainer.pgroup));
    if (pgroupPath.exists() && settingsContainer.value("usePgroupAsDefaultPath").toBool()){
         cwd.setPath(pgroupPath.path());
    } else {
        cwd.setPath(QDir::homePath());
    }

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(QDir::rootPath());
    uiFileBrowser->columnView->setModel(fileModel);
    uiFileBrowser->columnView->setRootIndex(fileModel->index(QDir::rootPath()));
    uiFileBrowser->columnView->selectionModel()->setCurrentIndex(fileModel->index(cwd.path()), QItemSelectionModel::Select);
    uiFileBrowser->columnView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(uiFileBrowser->pushButtonFileBrowser, SIGNAL (clicked()), this, SLOT (selectButton()));
    uiFileBrowser->columnView->selectionModel()->clearSelection();
}

void fileBrowser::selectButton(){
    getCurrentEntry();
    appendEntry();

    close();
}

void fileBrowser::appendEntry(){

    // check if current entry already exists in filelisting to avoid duplicates
    QStringList listWidgetEntries;
    for (int ii=0; ii<baseUi->ui->filelistingListWidget->count(); ii++){
        listWidgetEntries.push_back(baseUi->ui->filelistingListWidget->item(ii)->text());
    }

    // clear output
    baseUi->ui->filelistingListWidget->clear();
    for (int ii=0; ii<filePathQList.size(); ii++){
        listWidgetEntries.push_back(filePathQList[ii]);
    }

    // remove duplicates and write new output
    if (removeDuplicates){
        listWidgetEntries.removeDuplicates();
    }
    baseUi->ui->filelistingListWidget->addItems(listWidgetEntries);


}

void fileBrowser::getCurrentEntry(){
    filePathQList.clear();
    QModelIndex index = uiFileBrowser->columnView->currentIndex();
    QModelIndexList indexList = uiFileBrowser->columnView->selectionModel()->selection().indexes();
    qDebug() << indexList.size();
    for (int ii=0; ii<indexList.size(); ++ii) {
        filePath = "/" + indexList[ii].data().toString().toStdString();
        QModelIndex parentIndex, parentIndexOld;
        QVariant dataParent;
        QString dataParentString;
        parentIndexOld = index;
        std::string parentStdString;
        qDebug() << filePath;

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
        filePathQList.push_back(QString::fromStdString(filePath));
    }
    filePathQList.removeDuplicates(); // probably not needed

}

void fileBrowser::setParentDirActive(){
    QModelIndex index = uiFileBrowser->columnView->currentIndex();
    QModelIndex parentIndex = uiFileBrowser->columnView->model()->parent(index);
    uiFileBrowser->columnView->setCurrentIndex(parentIndex);
}


fileBrowser::~fileBrowser()
{
    delete uiFileBrowser;
    delete fileModel;
}
