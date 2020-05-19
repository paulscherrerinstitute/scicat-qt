#include "filedialogfilelisting.h"

FileDialogFilelisting::FileDialogFilelisting(QWidget *parent) : QFileDialog()
{
    m_btnOpen = NULL;
    m_listView = NULL;
    m_treeView = NULL;
    m_selectedFiles.clear();

    this->setOption(QFileDialog::DontUseNativeDialog, true);
//    this->setFileMode(QFileDialog::Directory);
    QList<QPushButton*> btns = this->findChildren<QPushButton*>();
    for (int i = 0; i < btns.size(); ++i) {
        QString text = btns[i]->text();
        if (text.toLower().contains("open") || text.toLower().contains("choose"))
        {
            m_btnOpen = btns[i];
            break;
        }
    }

    if (!m_btnOpen) return;

    m_btnOpen->installEventFilter(this);
    m_btnOpen->disconnect(SIGNAL(clicked()));
    connect(m_btnOpen, SIGNAL(clicked()), this, SLOT(chooseClicked()));


    m_listView = findChild<QListView*>("listView");
    if (m_listView) {
        m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    m_treeView = findChild<QTreeView*>();
    if (m_treeView) {
        m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    baseUi = dynamic_cast<Dialog*>(parent);
    QSettings settingsContainer("PSI", "SciCat");
    QDir cwd;
    QDir pgroupPath = QDir("/das/work/"+QString::fromStdString(baseUi->metadataContainer.pgroup).midRef(0,3)+"/"+QString::fromStdString(baseUi->metadataContainer.pgroup));
    if (pgroupPath.exists() && settingsContainer.value("usePgroupAsDefaultPath").toBool()){
         cwd.setPath(pgroupPath.path());
    } else {
        cwd.setPath(QDir::homePath());
    }
    setDirectory(cwd);
    QList<QUrl> drives;
    drives << QUrl::fromLocalFile(pgroupPath.path());
    drives << QUrl::fromLocalFile(cwd.absolutePath());
    setSidebarUrls(drives);
}

bool FileDialogFilelisting::eventFilter( QObject* watched, QEvent* event )
{
    QPushButton *btn = qobject_cast<QPushButton*>(watched);
    if (btn)
    {
        if(event->type()==QEvent::EnabledChange) {
            if (!btn->isEnabled()) {
                btn->setEnabled(true);
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}


void FileDialogFilelisting::chooseClicked()
{
    QModelIndexList indexList = m_listView->selectionModel()->selectedIndexes();
    foreach (QModelIndex index, indexList)
    {
        if (index.column()== 0)
        {
            m_selectedFiles.append(QDir(this->directory().absolutePath()).filePath(index.data().toString()));
        }
    }


    appendEntry();
    QDialog::accept();
}

QStringList FileDialogFilelisting::selectedFiles()
{
    return m_selectedFiles;
}

void FileDialogFilelisting::appendEntry(){

    // check if current entry already exists in filelisting to avoid duplicates
    QStringList listWidgetEntries;
    for (int ii=0; ii<baseUi->ui->filelistingListWidget->count(); ii++){
        listWidgetEntries.push_back(baseUi->ui->filelistingListWidget->item(ii)->text());
    }

    // clear output
    baseUi->ui->filelistingListWidget->clear();
    for (int ii=0; ii<m_selectedFiles.size(); ii++){
        listWidgetEntries.push_back(m_selectedFiles[ii]);
    }

    // remove duplicates and write new output
    if (removeDuplicates){
        listWidgetEntries.removeDuplicates();
    }
    baseUi->ui->filelistingListWidget->addItems(listWidgetEntries);


}
