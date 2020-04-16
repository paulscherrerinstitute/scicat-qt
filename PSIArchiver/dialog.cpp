#include "dialog.h"
#include "./ui_dialog.h"
#include "ui_filebrowser.h"
#include "qfiledialog.h"
#include "QMessageBox"
#include "ui_login.h"
#include "ui_datasetbrowser.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{

    ui->setupUi(this);
    int indx;
    indx = ui->stackedWidget->indexOf(ui->stackedWidget->currentWidget());


    // update number of slides/pages
    numSlides = ui->stackedWidget->count();

    // connect button signals to slots
    connect(ui->nextSlideButton, SIGNAL (clicked()), this, SLOT (nextSlide()));
    connect(ui->addFilesDirectoriesButton, SIGNAL (clicked()), this, SLOT (fileDialogFilelisting()));
    connect(ui->clearButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearFilelisting()));
    connect(ui->clearAllButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearAllFilelisting()));
    connect(ui->clearButtonDatasets, SIGNAL (clicked()), this, SLOT (clearDatasets()));
    connect(ui->clearAllButtonDatasets, SIGNAL (clicked()), this, SLOT (clearAllDatasets()));
    connect(ui->lineEditPgroup, SIGNAL (editingFinished()), this, SLOT (getPI()));
    connect(ui->pushButtonNavigationPgroup, SIGNAL (clicked()), this, SLOT (goToPgroup()));
    connect(ui->pushButtonNavigationFiles, SIGNAL (clicked()), this, SLOT (goToFiles()));
    connect(ui->pushButtonNavigationMetadata, SIGNAL (clicked()), this, SLOT (goToMetadata()));
    connect(ui->pushButtonNavigationSubmit, SIGNAL (clicked()), this, SLOT (goToSubmit()));
    connect(ui->pushButtonAddDatasets, SIGNAL (clicked()), this, SLOT (openDatasetBrowser()));
    connect(ui->comboBoxDatatype, SIGNAL (currentIndexChanged(int)), this, SLOT (comboBoxDatatypeChanged(int)));
    connect(ui->metadataGuided, SIGNAL (currentChanged(int)), this, SLOT (updateMetadataBrowser()));
    connect(ui->toolButtonPlus, SIGNAL(clicked()), this, SLOT(appendMetadataEntry()));
    connect(ui->toolButtonMinus, SIGNAL(clicked()), this, SLOT(removeMetadataEntry()));
//    connect(ui->lineEditOwner, SIGNAL (editingFinished()), this, SLOT(updateOwner()))

    // warning label
    ui->warningLabel->setVisible(false);

    // prepare function handles for nextSlideButton
    nextSlideButtonFunctions.push_back(&Dialog::PINextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::filelistingNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::metadataNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::submitNextSlide);

    // pgroup page
    ui->lineEditPIMail->setEnabled(false);
    ui->lineEditPIFirstname->setEnabled(false);
    ui->lineEditPILastname->setEnabled(false);
    ui->lineEditProposalID_2->setEnabled(false);

    // metadata page
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());

    // navigation buttons
    navigationButtons.push_back(ui->pushButtonNavigationPgroup);
    navigationButtons.push_back(ui->pushButtonNavigationFiles);
    navigationButtons.push_back(ui->pushButtonNavigationMetadata);
    navigationButtons.push_back(ui->pushButtonNavigationSubmit);

    // dataset selection
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->insertColumn(0);
    ui->tableWidget->insertColumn(1);
    ui->tableWidget->setColumnWidth(0, 200);
    ui->tableWidget->setColumnWidth(1, 200);
    ui->clearAllButtonDatasets->setEnabled(false);
    ui->clearButtonDatasets->setEnabled(false);


    // metadata editor
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->setColumnWidth(0, 150);
    ui->treeWidget->setColumnWidth(0, ui->treeWidget->width()-150);
    ui->treeWidget->invisibleRootItem()->setFlags((Qt::ItemFlags)32);

    addEntryMetadataTreeView("scientificMetadata", "", nullptr, false, 1);
    addEntryMetadataTreeView("sample", "", "scientificMetadata", false, 0);
    addEntryMetadataTreeView("description", "", "sample", false, 0);
    addEntryMetadataTreeView("name", "", "sample", false, 0);
    addEntryMetadataTreeView("principalInvestigator", QString::fromStdString(metadataContainer.PIemail), "sample", false, 0);


    styleSheetsContainer.buttonNavigation = "QPushButton{background-color:rgb(0, 45, 59); border-radius:0px}; "
                                          "QPushButton:hover{background-color:rgb(0, 70, 91); border-radius:0px}; "
                                          "QPushButton:!enabled{background-color:rgb(0, 45, 59); border-radius:0px; color:rgb(198, 198, 198)}";


}

void Dialog::sendWarning(QString warningMsg){
    ui->warningLabel->setText(warningMsg);
    ui->warningLabel->setVisible(true);
    QTimer::singleShot(5000, ui->warningLabel, &QLabel::hide);
}

void Dialog::keyPressEvent(QKeyEvent *event){
    if (event->key() != Qt::Key_Escape){
        QDialog::keyPressEvent(event);
    }
}

void Dialog::updateNavigationButton(){
    // get current slide index
    int slideIndex = ui->stackedWidget->currentIndex();
    for (int ii=0; ii<(int)navigationButtons.size(); ++ii) {
        if (ii==slideIndex+1 || ii==slideIndex){
            // set to active
            navigationButtons[ii]->setEnabled(true);
        } else if (ii>slideIndex+1) {
            // disable next steps
            navigationButtons[ii]->setEnabled(false);
        }
    }
    QPropertyAnimation *animationSlider = new QPropertyAnimation(ui->labelSlider, "geometry");
    animationSlider->setDuration(80);
    animationSlider->setStartValue(ui->labelSlider->geometry());
    animationSlider->setEndValue(QRect(20,80+slideIndex*70,5,70));
    animationSlider->start();

}

void Dialog::goToPgroup(){
    goToPage(0);
}

void Dialog::goToFiles(){
    if (checkPIInfo())
        goToPage(1);
}

void Dialog::goToMetadata(){
    if (checkPIInfo() && filelistingCheckAndUpdate()){
        goToPage(2);
    }
}

void Dialog::goToSubmit(){
    if (checkPIInfo() && filelistingCheckAndUpdate() && checkMetadata()){
        goToPage(3);
    }
}

void Dialog::goToPage(int pageIndex){
    int slideIndex = ui->stackedWidget->currentIndex();
    qDebug() <<  "current index: " << slideIndex;
    ui->stackedWidget->setCurrentIndex(pageIndex);
    updateNavigationButton();
}

void Dialog::PINextSlide(){
    qDebug() << "PI";
    if (checkPIInfo()){
        goToNextSlide();
    }
}

bool Dialog::checkMetadata(){
    return true;
}

bool Dialog::filelistingCheckAndUpdate(){
    if (ui->filelistingListWidget->count()==0){
        QMessageBox::information(this, tr("Invalid entry"), tr("Please add at least one file/directory.") );
        return false;
    }
    // prepare metadata entries
    ui->lineEditOwnerEmail->setText(QString::fromStdString(metadataContainer.ownerEmail));
    ui->lineEditOwner->setText(QString::fromStdString(metadataContainer.owner));
    ui->lineEditOwnerGroup->setText(QString::fromStdString(metadataContainer.ownerGroup));
    ui->lineEditProposalID->setText(QString::fromStdString(metadataContainer.proposalID));
    ui->lineEditInvestigator->setText(QString::fromStdString(metadataContainer.PIemail));

    return true;
}

void Dialog::filelistingNextSlide(){
    if (filelistingCheckAndUpdate()){
        goToNextSlide();
    }
}
void Dialog::metadataNextSlide(){
    std::cout << "metadata" << std::endl;
    goToNextSlide();
}
void Dialog::submitNextSlide(){
    std::cout << "submit" << std::endl;
    QMessageBox msgBox;
    msgBox.setText("Do you want to submit the dataset?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    switch (ret){
    case QMessageBox::Yes:
        // run datasetIngestor
        break;
    case QMessageBox::No:
        // no ingestion
        break;
    default:
        break;
    }
}


void Dialog::nextSlide(){
    // update currentSlideIndex
    currentSlideIndex = ui->stackedWidget->indexOf(ui->stackedWidget->currentWidget());
    (this->*nextSlideButtonFunctions[currentSlideIndex])();
}

void Dialog::goToNextSlide(){
    ui->stackedWidget->setCurrentIndex(++currentSlideIndex);
    updateNavigationButton();
}

bool Dialog::checkPgroup(){
    bool pgroupIsValid = true;
    if (ui->lineEditPgroup->text().size()==0){
        QMessageBox::information(this, tr("Error"), tr("pgroup cannot be empty") );
        pgroupIsValid = false;
    } else if ((ui->lineEditPgroup->text().size() != 6) || (ui->lineEditPgroup->text().toStdString().find("p") != 0)){
        QMessageBox::information(this, tr("Error"), tr("Please enter a valid pgroup") );
        pgroupIsValid = false;
    }

    // check if user has access to the given pgroup
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22fields%22%3A%20%22ownerGroup%22%7D&access_token=" + token + "'");
    if (pgroupIsValid && (tmp.find(ui->lineEditPgroup->text().toStdString())==std::string::npos)){
        auto msg = "Cannot access pgroup " + ui->lineEditPgroup->text().toStdString() + ". Permission denied.";
        QMessageBox::information(this, tr("Error"), tr(msg.c_str()));
        pgroupIsValid = false;
    }

    return pgroupIsValid;
}

bool Dialog::checkPIInfo(){

    std::cout << "checking PI" << std::endl;
    return checkPgroup();

}

void Dialog::getPI(){
    if (Dialog::checkPgroup()){
        metadataContainer.pgroup = ui->lineEditPgroup->text().toStdString();
        auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.pgroup + "%22%7D%7D&access_token=" + token + "'");
//        std::cout << tmp << std::endl;
        if (tmp.length()>5){
            metadataContainer.PIFirstname = utils::extractFromJson(tmp, "pi_firstname");
            metadataContainer.PILastname = utils::extractFromJson(tmp, "pi_lastname");
            metadataContainer.PIemail = utils::extractFromJson(tmp, "pi_email");
            metadataContainer.proposalTitle = utils::extractFromJson(tmp, "title");
            metadataContainer.proposalAbstract = utils::extractFromJson(tmp, "abstract");
            metadataContainer.instrument = utils::extractFromJson(tmp, "instrument");
            metadataContainer.proposalID = utils::extractFromJson(tmp, "proposalId");

            // set defaults for owner/ownergroup etc
            metadataContainer.ownerGroup = metadataContainer.pgroup;
            metadataContainer.owner = metadataContainer.PIFirstname + " " + metadataContainer.PILastname;
            metadataContainer.ownerEmail = metadataContainer.PIemail;
            metadataContainer.investigator = metadataContainer.PIemail;

            ui->lineEditPIMail->setText(QString::fromStdString(metadataContainer.PIemail));
            ui->lineEditPIFirstname->setText(QString::fromStdString(metadataContainer.PIFirstname));
            ui->lineEditPILastname->setText(QString::fromStdString(metadataContainer.PILastname));
            ui->lineEditProposalID_2->setText(QString::fromStdString(metadataContainer.proposalID));


        }

        ui->lineEditPIMail->setEnabled(true);
        ui->lineEditPIFirstname->setEnabled(true);
        ui->lineEditPILastname->setEnabled(true);
        ui->lineEditProposalID_2->setEnabled(true);
    }
}

void Dialog::fileDialogFilelisting(){
    fileBrowser fb(this);
    fb.connectToUi(ui);
    fb.exec();

    ui->filelistingListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void Dialog::clearFilelisting(){
    // get current item
    std::cout << ui->filelistingListWidget->currentItem()->text().toStdString() << std::endl;
    ui->filelistingListWidget->removeItemWidget(ui->filelistingListWidget->currentItem());

    ui->filelistingListWidget->setUpdatesEnabled(false);


    QModelIndexList indexes;
    while((indexes = ui->filelistingListWidget->selectionModel()->selectedIndexes()).size()) {
        ui->filelistingListWidget->model()->removeRow(indexes.first().row());
    }
    ui->filelistingListWidget->setUpdatesEnabled(true);

}

void Dialog::clearAllDatasets(){
    for (int ii=0; ii<ui->tableWidget->rowCount(); ++ii) {
        ui->tableWidget->removeRow(ui->tableWidget->rowCount()-ii-1);
    }
}

void Dialog::clearDatasets(){
    QModelIndexList indexList = ui->tableWidget->selectionModel()->selectedRows();
    for (int ii=0; ii<indexList.size(); ++ii) {
        ui->tableWidget->removeRow(indexList[ii].row());
    }

}

void Dialog::clearAllFilelisting(){
    ui->filelistingListWidget->clear();
}

void Dialog::updateFilelistingDisplay(){
    ui->filelistingListWidget->clear();
    for (uint containerIndex=0; containerIndex<filelistingContainer.size(); containerIndex++){
        ui->filelistingListWidget->addItem(filelistingContainer[containerIndex]);
    }

}

QStringList Dialog::openFileBrowser(){
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setViewMode(QFileDialog::List);
//    dialog.AcceptOpen
    QStringList fileName;
    if (dialog.exec()){
        fileName = dialog.selectedFiles();
    }
    std::cout << fileName[0].toStdString() << std::endl;
//    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),"~/", QFileDialog::Directory);
    return fileName;
}

void Dialog::openDatasetBrowser(){
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%20%22" + metadataContainer.pgroup + "%22%7D%2C%20%22fields%22%3A%20%7B%22datasetName%22%3Atrue%2C%20%22pid%22%3Atrue%7D%7D&access_token=" + token + "'");
    auto delim = '}';
    std::vector<std::string> tmpVec = utils::split(tmp, delim);

    datasetContainer.clear();
    for (auto v: tmpVec) {
        std::cout << v << std::endl;
        QStringList tmpQStringList;
        if ((v.find("datasetName")!=std::string::npos) && (v.find("pid")!=std::string::npos)){
            tmpQStringList.push_back(QString::fromStdString(utils::extractFromJson(v, "datasetName")));
            tmpQStringList.push_back(QString::fromStdString(utils::extractFromJson(v, "pid")));
        }

        if (tmpQStringList.size()==2)
            datasetContainer.push_back(tmpQStringList);
    }
//    datasets_m->setContainer(datasetContainer);

//    ui->tableViewDatasetBrowser->model

    // open DatasetBrowser
    DatasetBrowser* db = new DatasetBrowser(datasetContainer, this);
    db->connectToUi(this);
    db->exec();
    delete db;

}

void Dialog::addEntryMetadataTreeView(QString name, QString description, QTreeWidgetItem *parent, bool setFocus, int flag){
    QTreeWidgetItem *treeitem;
    if (parent==nullptr){
        treeitem = new QTreeWidgetItem(ui->treeWidget);
    } else {
        treeitem = new QTreeWidgetItem();
    }
    treeitem->setText(0, name);
    treeitem->setText(1, description);
    if (flag==0){
        treeitem->setFlags(treeitem->flags() ^ Qt::ItemIsDragEnabled);
        treeitem->setFlags(treeitem->flags() ^ Qt::ItemIsDropEnabled);
        treeitem->setFlags(treeitem->flags() | Qt::ItemIsEditable);
    } else if (flag==1){
        treeitem->setFlags(treeitem->flags() | Qt::ItemIsDropEnabled);
        treeitem->setFlags(treeitem->flags() | Qt::ItemIsEditable);
    } else {
        treeitem->setFlags(treeitem->flags() | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled);
        treeitem->setFlags(treeitem->flags() | Qt::ItemIsEditable);
    }

    if (parent!=nullptr){
        parent->addChild(treeitem);
        if (setFocus){
            parent->setExpanded(true);
        }
    }

    if (setFocus){
        treeitem->setSelected(true);
        ui->treeWidget->editItem(treeitem, 0);
    }

}

void Dialog::addEntryMetadataTreeView(QString name, QString description, QString parent, bool setFocus, int flag){
    if (parent.length()==0){
        addEntryMetadataTreeView(name, description, nullptr, setFocus, flag);
        return;
    }

    auto res = ui->treeWidget->findItems(parent, Qt::MatchExactly|Qt::MatchRecursive, 0);
    if (res.size()==0){
        std::cout << parent.toStdString() << std::endl;
        std::cout << "Could not find parent entry." << std::endl;
        return;
    }

    addEntryMetadataTreeView(name, description, res[0], setFocus, flag);

}

void Dialog::addEntryMetadataTreeView(QString name, QString description){
    addEntryMetadataTreeView(name, description, nullptr, false, 0);
}

void Dialog::comboBoxDatatypeChanged(int val){
        ui->lineEditInvestigator->setEnabled(!(bool)val);
        ui->textEditUsedSoftware->setEnabled(!(bool)val);
        ui->metadataGuided->setTabEnabled(1,!(bool)val);
}

void Dialog::updateMetadataBrowser(){
    updateMetadataEntry("principalInvestigator", QString::fromStdString(metadataContainer.PIemail), 0);
    updateMetadataEntry("type", ui->comboBoxDatatype->currentText(), 0);
    updateMetadataEntry("ownerGroup", QString::fromStdString(metadataContainer.ownerGroup), 0);
    updateMetadataEntry("owner", QString::fromStdString(metadataContainer.owner), 0);
    updateMetadataEntry("ownerEmail", QString::fromStdString(metadataContainer.ownerEmail), 0);
    updateMetadataEntry("datasetName", QString::fromStdString(metadataContainer.datasetName), 0);

}

void Dialog::appendMetadataEntry(){
    // check if item supports children
    if (ui->treeWidget->currentItem()->flags().testFlag(Qt::ItemIsDropEnabled)){
        addEntryMetadataTreeView("name", "value", ui->treeWidget->currentItem(), true, 2);
    } else {
        sendWarning("Cannot append child entry to protected field.");
    }
}

void Dialog::removeMetadataEntry(){
    // check if item is protected
    if (ui->treeWidget->currentItem()->flags()>35){
        std::cout << "here" << std::endl;
        ui->treeWidget->removeItemWidget(ui->treeWidget->currentItem(), 0);
        QModelIndexList indexList = ui->treeWidget->selectionModel()->selectedRows();
        for (int ii=0; ii<indexList.size(); ii++){
            ui->treeWidget->model()->removeRow(indexList[ii].row());
        }
    } else {
        sendWarning("Cannot remove protected field.");
    }

}

void Dialog::updateMetadataEntry(QString name, QString targetValue, int flag){
    auto res = ui->treeWidget->findItems(name, Qt::MatchExactly, 0);
    if (res.size()>0){
        res[0]->setText(1, targetValue);
        if (flag==0){
            res[0]->setFlags((Qt::ItemFlags)35);
        } else if (flag==1) {
            res[0]->setFlags((Qt::ItemFlags)43);
        } else {
            res[0]->setFlags((Qt::ItemFlags)47);
        }
    } else {
        addEntryMetadataTreeView(name, targetValue);
    }
}

Dialog::~Dialog()
{
    delete datasets_m;
    delete ui;
}





fileBrowser::fileBrowser(QWidget *parent) :
    QDialog(parent),
    uiFileBrowser(new Ui::fileBrowser)
{
    uiFileBrowser->setupUi(this);

    QString sPath = "/";
    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(sPath);
    uiFileBrowser->columnView->setModel(fileModel);
    uiFileBrowser->columnView->setRootIndex(fileModel->index("/"));
    uiFileBrowser->columnView->selectionModel()->setCurrentIndex(fileModel->index("/home/wakonig_k"), QItemSelectionModel::Select);
    uiFileBrowser->columnView->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    uiFileBrowser->columnView->show();

    connect(uiFileBrowser->pushButtonFileBrowser, SIGNAL (clicked()), this, SLOT (selectButton()));
//    connect(uiFileBrowser->columnView->selectionModel(), SIGNAL (doubleClicked()), this, SLOT (setChildDirActive()));
    connect(uiFileBrowser->toolButtonUp, SIGNAL (clicked()), this, SLOT (setParentDirActive()));
    uiFileBrowser->columnView->selectionModel()->clearSelection();
}

void fileBrowser::connectToUi(Ui::Dialog *ui){
    baseUi = ui;
}

void fileBrowser::selectButton(){
    getCurrentEntry();
    appendEntry();
//    updateFileCount(baseUi);

    close();
}

void fileBrowser::appendEntry(){

    // check if current entry already exists in filelisting to avoid duplicates
    QStringList listWidgetEntries;
    for (int ii=0; ii<baseUi->filelistingListWidget->count(); ii++){
        listWidgetEntries.push_back(baseUi->filelistingListWidget->item(ii)->text());
    }

    // clear output
    baseUi->filelistingListWidget->clear();
    for (int ii=0; ii<filePathQList.size(); ii++){
        listWidgetEntries.push_back(filePathQList[ii]);
    }

    // remove duplicates and write new output
    if (removeDuplicates){
        listWidgetEntries.removeDuplicates();
    }
    baseUi->filelistingListWidget->addItems(listWidgetEntries);


}

void fileBrowser::getCurrentEntry(){
    filePathQList.clear();
    QModelIndex index = uiFileBrowser->columnView->currentIndex();
    QModelIndexList indexList = uiFileBrowser->columnView->selectionModel()->selection().indexes();
    std::cout << indexList.size() << std::endl;
    for (int ii=0; ii<indexList.size(); ++ii) {
        filePath = "/" + indexList[ii].data().toString().toStdString();
        QModelIndex parentIndex, parentIndexOld;
        QVariant dataParent;
        QString dataParentString;
        parentIndexOld = index;
        std::string parentStdString;
        std::cout << filePath << std::endl;

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


void fileBrowser::setChildDirActive(){
    std::cout << "Child" << std::endl;

}


fileBrowser::~fileBrowser()
{
    delete uiFileBrowser;
    delete fileModel;
}



Login::Login(QWidget *parent) :
    QDialog(parent),
    uiLogin(new Ui::Login)
{
    uiLogin->setupUi(this);

    // connect to login UI
    connect(uiLogin->pushButtonLogin, SIGNAL (clicked()), this, SLOT (getLoginData()));

}


void Login::connectToGui(Dialog *ui){
    baseUi = ui;
}

void Login::getLoginData(){
    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();
    std::string token;
    auto tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' 'https://dacat.psi.ch/auth/msad'");
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        QMessageBox::information(this, tr("Authentication error"), tr("Please check your username and password") );
        uiLogin->lineEditPassword->clear();
    } else {
        accessGranted = true;
        if (tmp.find("access_token") != std::string::npos){
            token = utils::extractFromJson(tmp, "access_token");
        } else if (tmp.find("id") != std::string::npos) {
            token = utils::extractFromJson(tmp, "id");
        } else {
            std::cout << tmp << std::endl;
            QMessageBox::information(this, tr("Connection error"), tr("Could not connect to SciCat") );
            accessGranted = false;
        }
        if (accessGranted){
            close();
            baseUi->show();
            baseUi->token = token;
            std::cout << token << std::endl;
        }

    }

}


Login::~Login()
{
    delete uiLogin;
}


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
}

void DatasetBrowser::selectButton(){
    QModelIndexList indexList = ui->tableViewDatasetBrowser->selectionModel()->selectedRows();
    std::cout << indexList.size() << std::endl;
    for (int ii=0; ii<indexList.size(); ++ii) {
        QStringList tmp;
        baseUi->ui->tableWidget->insertRow(0);
        for (int colIndx=0; colIndx<datasets_m->columnCount(); colIndx++) {
            QTableWidgetItem *tmpItem = new QTableWidgetItem(datasets_m->data(indexList[ii].siblingAtColumn(colIndx),0).toString());
            baseUi->ui->tableWidget->setItem(0,colIndx,tmpItem);
            baseUi->ui->clearAllButtonDatasets->setEnabled(true);
            baseUi->ui->clearButtonDatasets->setEnabled(true);
        }

    }
    close();

}

void DatasetBrowser::connectToUi(Dialog *uiDialog){
    baseUi = uiDialog;
}

DatasetBrowser::~DatasetBrowser()
{
    delete datasets_m;
    delete ui;
}








