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


    navigationAnimationGroup = new QSequentialAnimationGroup;
    animationNavigationButtons = new QPropertyAnimation(ui->frameRetrieve, "geometry");
    animationCover = new QPropertyAnimation(ui->frameRetrieveButtons, "geometry");

    animationNavigationButtons->setDuration(800);
    auto startRect = ui->frameRetrieve->geometry();
    animationNavigationButtons->setStartValue(QRect(startRect.left(),300,startRect.width(),startRect.height()));

    animationCover->setDuration(800);
//    auto retrieveCoverLoc = ui->frameRetrieveButtons->geometry();
    auto retrieveButtonLoc = ui->pushButtonGoToRetrieve->geometry();
    animationCover->setStartValue(QRect(retrieveButtonLoc.left(),retrieveButtonLoc.bottom(),retrieveButtonLoc.width(),0));
    animationNavigationButtons->setEndValue(QRect(startRect.left(),29,startRect.width(),startRect.height()));
    animationNavigationButtons->setEasingCurve(QEasingCurve::OutExpo);

    animationCover->setEndValue(QRect(retrieveButtonLoc.left(), retrieveButtonLoc.bottom(), retrieveButtonLoc.width(), 140));
    animationCover->setEasingCurve(QEasingCurve::OutExpo);

    navigationAnimationGroup->addAnimation(animationNavigationButtons);
    navigationAnimationGroup->addAnimation(animationCover);


    // update number of slides/pages
    numSlides = ui->stackedWidget->count();

    ui->stackedWidget->setCurrentIndex(0);
    ui->metadataGuided->setCurrentIndex(0);

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
    connect(ui->lineEditPgroup, SIGNAL(editingFinished()), this, SLOT(piChanged()));
    connect(ui->pushButtonRestart, SIGNAL(clicked()), this, SLOT(restart()));
    connect(ui->lineEditDatasetName, SIGNAL(editingFinished()), this, SLOT(datasetNameCheck()));
    connect(ui->lineEditInvestigator, SIGNAL(editingFinished()), this, SLOT(investigatorChanged()));
    connect(ui->textEditDescription, SIGNAL(textChanged()), this, SLOT(descriptionChanged()));
    connect(ui->textEditUsedSoftware, SIGNAL(textChanged()), this, SLOT(usedSoftwareChanged()));
    connect(ui->pushButtonGoToArchive, SIGNAL(clicked()), this, SLOT(goToArchive()));
    connect(ui->pushButtonGoToRetrieve, SIGNAL(clicked()), this, SLOT(goToRetrieve()));
//    connect(ui->lineEditOwner, SIGNAL (editingFinished()), this, SLOT(updateOwner()))

    // warning label
    ui->warningLabel->setVisible(false);

    // prepare function handles for nextSlideButton
    nextSlideButtonFunctions.push_back(&Dialog::PINextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::filelistingNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::metadataNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::submitNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::quit);


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
    ui->tableWidget->setHorizontalHeaderLabels(QStringList({"Name", "ID"}));
    ui->clearAllButtonDatasets->setEnabled(false);
    ui->clearButtonDatasets->setEnabled(false);


    // metadata editor
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->setColumnWidth(0, 150);
    ui->treeWidget->setColumnWidth(0, ui->treeWidget->width()-150);
    ui->treeWidget->invisibleRootItem()->setFlags((Qt::ItemFlags)32);
    ui->treeWidget->setHeaderLabels(QStringList({"Name", "Value"}));

    addEntryMetadataTreeView("scientificMetadata", "", nullptr, false, 2);
    addEntryMetadataTreeView("sample", "", "scientificMetadata", false, 1);
    addEntryMetadataTreeView("description", "", "sample", false, 1);
    addEntryMetadataTreeView("name", "", "sample", false, 1);
    addEntryMetadataTreeView("principalInvestigator", QString::fromStdString(metadataContainer.PIemail), "sample", false, 1);

//    labelSlider = new QLabel(this);
//    labelSlider->setGeometry(QRect(0,ui->pushButtonNavigationPgroup->geometry().top()+79,5,ui->pushButtonNavigationPgroup->geometry().height()));
//    labelSlider->setStyleSheet("QLabel{background-color: rgb(0, 153, 200)}");

    ui->pushButtonRestart->setVisible(false);
    if (!movie->isValid())
        return;


    ui->tableWidgetRetriever->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetRetriever->insertColumn(0);
    ui->tableWidgetRetriever->insertColumn(1);
    ui->tableWidgetRetriever->insertColumn(2);
    ui->tableWidgetRetriever->setColumnWidth(0, 200);
    ui->tableWidgetRetriever->setColumnWidth(1, 200);
    ui->tableWidgetRetriever->setColumnWidth(2, 200);
    ui->tableWidgetRetriever->setHorizontalHeaderLabels(QStringList({"Name", "Retrievable", "Status"}));


    styleSheetsContainer.buttonNavigation = "QPushButton{background-color:rgb(0, 45, 59); border-radius:0px}; "
                                          "QPushButton:hover{background-color:rgb(0, 70, 91); border-radius:0px}; "
                                          "QPushButton:!enabled{background-color:rgb(0, 45, 59); border-radius:0px; color:rgb(198, 198, 198)}";

}

void Dialog::sendWarning(QString warningMsg){
    ui->warningLabel->setText(warningMsg);
    ui->warningLabel->setVisible(true);
    QTimer::singleShot(5000, ui->warningLabel, &QLabel::hide);
}

void Dialog::toggleNavigationBar(int duration){


    navigationAnimationGroup->setDirection((QAbstractAnimation::Direction)archiveCollapsed);
    animationNavigationButtons->setDuration(duration);
    animationCover->setDuration(duration);
    navigationAnimationGroup->start();
    archiveCollapsed = !archiveCollapsed;

}

void Dialog::goToArchive(){
    if (archiveCollapsed){
        toggleNavigationBar(800);
    }
}

void Dialog::goToRetrieve(){
    if (!archiveCollapsed){
        toggleNavigationBar(800);
        updateRetrieveList();
        ui->stackedWidget->setCurrentIndex(5);
    }
}

void Dialog::keyPressEvent(QKeyEvent *event){
    if (event->key() != Qt::Key_Escape){
        QDialog::keyPressEvent(event);
    }
}

void Dialog::datasetNameCheck(){
    ui->lineEditDatasetName->setText(ui->lineEditDatasetName->text().replace(" ", "_"));
}

void Dialog::investigatorChanged(){
    metadataContainer.investigator = ui->lineEditInvestigator->text();
}

void Dialog::descriptionChanged(){
    metadataContainer.description = ui->textEditDescription->toPlainText();
    qDebug()<<metadataContainer.description;
}

void Dialog::usedSoftwareChanged(){
    metadataContainer.usedSoftware = ui->textEditUsedSoftware->toPlainText();
}

void Dialog::getAvailPgroups(){
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22fields%22%3A%20%22ownerGroup%22%7D&access_token=" + token + "'");
    auto delim = '}';
    std::vector<std::string> tmpVec = utils::split(tmp, delim);
    for (auto v:tmpVec){
        if (v.length()>5){
            availPgroups.push_back(QString::fromStdString(utils::extractFromJson(v, "ownerGroup")));
        }
    }

    // set autocomplete for pgroup selection
    pgroupCompleter = new QCompleter(availPgroups, this);
    pgroupCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->lineEditPgroup->setCompleter(pgroupCompleter);
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
//    QPropertyAnimation *animationSlider = new QPropertyAnimation(labelSlider, "geometry");
//    animationSlider->setDuration(80);
//    animationSlider->setStartValue(labelSlider->geometry());
//    animationSlider->setEndValue(QRect(0,ui->pushButtonNavigationPgroup->geometry().top()+79+slideIndex*70,5,ui->pushButtonNavigationPgroup->geometry().height()));
//    animationSlider->start();

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

void Dialog::piChanged(){
    PIHasChanged = true;
}

bool Dialog::checkMetadata(){
    if (ui->lineEditDatasetName->text().isEmpty()){
        QMessageBox::information(this, tr("Invalid entry"), tr("Dataset name cannot be empty.") );
        ui->metadataGuided->setCurrentIndex(0);
        ui->labelDatasetName->setFocus();
        return false;
    }
    metadataContainer.datasetName = ui->lineEditDatasetName->text();
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
    ui->lineEditInvestigator->setText(metadataContainer.investigator);

    filelistingContainer.clear();
    for (int ii=0; ii<ui->filelistingListWidget->count(); ii++ ){
        filelistingContainer.push_back(ui->filelistingListWidget->item(ii)->text());
    }

    return true;
}

void Dialog::filelistingNextSlide(){
    qDebug() << "filelisting";
    if (filelistingCheckAndUpdate()){
        goToNextSlide();
    }
}
void Dialog::metadataNextSlide(){
    qDebug() << "metadata";
    if (checkMetadata()){
        goToNextSlide();
    }
}
void Dialog::submitNextSlide(){
    qDebug() << "submit";
    QMessageBox msgBox;
    msgBox.setText("Do you want to submit the dataset?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    switch (ret){
    case QMessageBox::Yes:
        // run datasetIngestor
        ui->stackedWidget->setCurrentIndex(++currentSlideIndex);
        ui->nextSlideButton->setText("Done");
        startIngestion();
        break;
    case QMessageBox::No:
        // no ingestion
        break;
    default:
        break;
    }
}

void Dialog::setButtonsEnabled(bool enabled){
    ui->nextSlideButton->setEnabled(enabled);
    ui->pushButtonNavigationPgroup->setEnabled(enabled);
    ui->pushButtonNavigationFiles->setEnabled(enabled);
    ui->pushButtonNavigationMetadata->setEnabled(enabled);
    ui->pushButtonNavigationSubmit->setEnabled(enabled);
}

void Dialog::startIngestion(){
    ui->waitingCircle->setMovie(movie);
    ui->waitingCircle->show();
    movie->start();

    // disable buttons
    setButtonsEnabled(false);

    // start ingestion
    qDebug() << "starting ingestion";
    ingestor_m = new Ingestor("sleep 5;");
    connect(ingestor_m, SIGNAL(finished()), this, SLOT(ingestionFinished()));
    ingestor_m->connectToUi(this);
    ingestor_m->prepareIngestion();
    ingestor_m->start();

}

void Dialog::ingestionFinished(){
    // ingestion finished, reenable buttons
    movie->stop();
    ui->waitingCircle->hide();
    ui->pleaseWait->setText("Done");
    ui->pushButtonRestart->setVisible(true);
    ui->nextSlideButton->setEnabled(true);
}

void Dialog::resetContent(){
    setButtonsEnabled(true);
    filelistingContainer.clear();
    clearAllDatasets();
    clearAllFilelisting();
    ui->metadataGuided->setCurrentIndex(0);
    ui->nextSlideButton->setText("Next");
}

void Dialog::restart(){
    resetContent();

    goToPage(0); // go back to PI
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
    if (PIHasChanged){
        qDebug() << "checking PI";
        PIHasChanged = false;
        return checkPgroup();
    }

    return true;

}

void Dialog::getPI(){
    if (Dialog::checkPgroup()){
        metadataContainer.pgroup = ui->lineEditPgroup->text().toStdString();
        auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.pgroup + "%22%7D%7D&access_token=" + token + "'");
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
            auto tmpUser = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Users/userInfos?access_token=" + token + "'");
            defaultSettings.investigatorIsPI ? metadataContainer.investigator=QString::fromStdString(metadataContainer.PIemail) : metadataContainer.investigator=QString::fromStdString(utils::extractFromJson(tmpUser, "currentUserEmail"));
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
    QModelIndexList indexList = ui->filelistingListWidget->selectionModel()->selectedRows();

    for (int ii = 0; ii < indexList.size(); ++ii) {
        ui->filelistingListWidget->model()->removeRow(indexList[ii].row());
    }

}

void Dialog::clearAllDatasets(){
    int rowCount = ui->tableWidget->rowCount();
    for (int ii=0; ii<rowCount; ++ii) {
        ui->tableWidget->removeRow(ui->tableWidget->rowCount()-1);
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

    return fileName;
}

void Dialog::openDatasetBrowser(){
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%20%22" + metadataContainer.pgroup + "%22%7D%2C%20%22fields%22%3A%20%7B%22datasetName%22%3Atrue%2C%20%22pid%22%3Atrue%7D%7D&access_token=" + token + "'");
    auto delim = '}';
    std::vector<std::string> tmpVec = utils::split(tmp, delim);

    datasetContainer.clear();
    for (auto v: tmpVec) {
        qDebug() << v;
        QStringList tmpQStringList;
        if ((v.find("datasetName")!=std::string::npos) && (v.find("pid")!=std::string::npos)){
            tmpQStringList.push_back(QString::fromStdString(utils::extractFromJson(v, "datasetName")));
            tmpQStringList.push_back(QString::fromStdString(utils::extractFromJson(v, "pid")));
        }

        if (tmpQStringList.size()==2)
            datasetContainer.push_back(tmpQStringList);
    }

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
    updateItemFlag(treeitem, flag);

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
        qDebug() << parent.toStdString();
        qDebug() << "Could not find parent entry.";
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
//    updateMetadataEntry("principalInvestigator", QString::fromStdString(metadataContainer.PIemail), 0);
//    updateMetadataEntry("type", ui->comboBoxDatatype->currentText(), 0);
//    updateMetadataEntry("ownerGroup", QString::fromStdString(metadataContainer.ownerGroup), 0);
//    updateMetadataEntry("owner", QString::fromStdString(metadataContainer.owner), 0);
//    updateMetadataEntry("ownerEmail", QString::fromStdString(metadataContainer.ownerEmail), 0);
//    updateMetadataEntry("datasetName", metadataContainer.datasetName, 0);

}

void Dialog::appendMetadataEntry(){
    // check if item supports children
    if (ui->treeWidget->currentItem()->flags().testFlag(Qt::ItemIsDropEnabled)){
        ui->treeWidget->currentItem()->setData(1,0,"");
        addEntryMetadataTreeView("name", "value", ui->treeWidget->currentItem(), true, 4);

    } else {
        sendWarning("Cannot append child entry to protected field.");
    }
}

void Dialog::removeMetadataEntry(){
    // check if item is protected
    qDebug()<<ui->treeWidget->currentItem()->flags();
    if (ui->treeWidget->currentItem()->flags()>43){
        QModelIndexList indexList = ui->treeWidget->selectionModel()->selectedRows();
            qDebug() << ui->treeWidget->model()->data(indexList[0]);
            qDebug() << indexList[0].row();
            ui->treeWidget->currentItem()->parent()->removeChild(ui->treeWidget->currentItem());
//        }
    } else {
        sendWarning("Cannot remove protected field.");
    }

}

void Dialog::updateItemFlag(QTreeWidgetItem *item, int flag){
    if (flag==0){
        item->setFlags((Qt::ItemFlags)33); // selectable, enabled
    } else if (flag==1) {
        item->setFlags((Qt::ItemFlags)35); // selectable, enabled, editable
    } else if (flag==2) {
        item->setFlags((Qt::ItemFlags)41); // selectable, enabled, dropenabled
    } else if (flag==3) {
        item->setFlags((Qt::ItemFlags)43); // selectable, enabled, dropenabled, editable
    } else {
        item->setFlags((Qt::ItemFlags)47); // selectable, enabled, dropenabled, dragenabled, editable
    }
}

void Dialog::updateMetadataEntry(QString name, QString targetValue, int flag){
    auto res = ui->treeWidget->findItems(name, Qt::MatchExactly, 0);
    if (res.size()>0){
        res[0]->setText(1, targetValue);
        updateItemFlag(res[0], flag);
    } else {
        addEntryMetadataTreeView(name, targetValue);
    }
}

void Dialog::updateRetrieveList(){
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%22" + metadataContainer.pgroup + "%22%7D%2C%20%22fields%22%3A%7B%22datasetlifecycle%22%3A%22true%22%2C%20%22datasetName%22%3A%20%22true%22%7D%7D&access_token=" + token + "'");
    auto delim = '}';
    retrieveContainer.clear();
    std::vector<std::string> tmpVec = utils::split(tmp, delim);
    for (auto v : tmpVec) {
        qDebug() << v;
        QStringList tmpQStringList;
        if ((v.find("datasetName")!=std::string::npos) && (v.find("retrievable")!=std::string::npos) && (v.find("retrieveStatusMessage")!=std::string::npos)){
            ui->tableWidgetRetriever->insertRow(0);
            QTableWidgetItem *tmpItemDataset = new QTableWidgetItem(QString::fromStdString(utils::extractFromJson(v, "datasetName")));
            ui->tableWidgetRetriever->setItem(0,0,tmpItemDataset);
            QTableWidgetItem *tmpItemRetrievable = new QTableWidgetItem(QString::fromStdString(utils::extractFromJson(v, "retrievable")));
            ui->tableWidgetRetriever->setItem(0,1,tmpItemRetrievable);
            QTableWidgetItem *tmpItemStatus = new QTableWidgetItem(QString::fromStdString(utils::extractFromJson(v, "retrieveStatusMessage")));
            ui->tableWidgetRetriever->setItem(0,2,tmpItemStatus);
        }
        if (tmpQStringList.size()==3)
            datasetContainer.push_back(tmpQStringList);
    }


//    // open DatasetBrowser
//    DatasetBrowser* db = new DatasetBrowser(datasetContainer, this);
//    db->connectToUi(this);
//    db->exec();
//    delete db;

}



void Dialog::quit(){
    close();
}


Dialog::~Dialog()
{
    delete datasets_m;
    delete ui;
    delete animationNavigationButtons;
    delete animationCover;
}





fileBrowser::fileBrowser(QWidget *parent) :
    QDialog(parent),
    uiFileBrowser(new Ui::fileBrowser)
{
    uiFileBrowser->setupUi(this);

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(QDir::rootPath());
    uiFileBrowser->columnView->setModel(fileModel);
    uiFileBrowser->columnView->setRootIndex(fileModel->index(QDir::rootPath()));
    uiFileBrowser->columnView->selectionModel()->setCurrentIndex(fileModel->index(QDir::homePath()), QItemSelectionModel::Select);
    uiFileBrowser->columnView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(uiFileBrowser->pushButtonFileBrowser, SIGNAL (clicked()), this, SLOT (selectButton()));
    uiFileBrowser->columnView->selectionModel()->clearSelection();
}

void fileBrowser::connectToUi(Ui::Dialog *ui){
    baseUi = ui;
}

void fileBrowser::selectButton(){
    getCurrentEntry();
    appendEntry();

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
    baseUi->toggleNavigationBar(1);
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
            QMessageBox::information(this, tr("Connection error"), tr("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.") );
            accessGranted = false;
        }
        if (accessGranted){
            close();
            baseUi->toggleNavigationBar(1);
            baseUi->token = token;
            baseUi->getAvailPgroups();
            baseUi->show();
            qDebug() << (baseUi->navigationAnimationGroup->state()==QAbstractAnimation::Running);
//            while (baseUi->navigationAnimationGroup->state()==QAbstractAnimation::Running){
//                sleep(1);
//            }
//            baseUi->toggleNavigationBar(1);



//            baseUi->labelSlider->setGeometry(QRect(0,47,5,70));
            qDebug() << token;
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
        }
        baseUi->ui->clearAllButtonDatasets->setEnabled(true);
        baseUi->ui->clearButtonDatasets->setEnabled(true);

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


void Ingestor::prepareIngestion(){

    QString dirpath = baseUi->ingestionContainer.logPath + baseUi->metadataContainer.datasetName + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            baseUi->sendWarning("Could not write to output directory.");
        }
    }

    QFile metadataFile(QDir(dirpath).filePath(baseUi->ingestionContainer.filenameMetadata));
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        baseUi->sendWarning("Could not open metadata file.");
        return;
    }

    // prepare metadata file
    QJsonObject metadata;
    bool isRawData = (bool) baseUi->ui->comboBoxDatatype->currentIndex();
    if (isRawData){
        qDebug() << "Preparing metadata file for raw data";
        metadata["principalInvestigator"] = QString::fromStdString(baseUi->metadataContainer.PIemail); // this should be changed in the future; there is no need to save it as std::string
//        metadata["creationLocation"] = baseUi->metadataContainer.creationLocation;
        metadata["owner"] = QString::fromStdString(baseUi->metadataContainer.owner);
        metadata["ownerGroup"] = QString::fromStdString(baseUi->metadataContainer.ownerGroup);
        metadata["ownerEmail"] = QString::fromStdString(baseUi->metadataContainer.ownerEmail);
        metadata["datasetName"] = baseUi->metadataContainer.datasetName;
        metadata["type"] = "raw";
        metadata["description"] = baseUi->metadataContainer.description;
        auto res = baseUi->ui->treeWidget->findItems("scientificMetadata", Qt::MatchFlag::MatchExactly);
        QJsonArray scMetadata;
        for (int ii=0; ii<res.size(); ii++){
            qDebug() << res[ii]->childCount();
            scMetadata.push_back(treeModel2Json(res[ii]));
        }
        metadata["scientificMetadata"] = scMetadata;
        qDebug() << baseUi->metadataContainer.datasetName;
    } else {
        qDebug() << "Preparing metadata file for derived data";
        metadata["sourceFolder"] = extractSourceFolder(baseUi->filelistingContainer);
        metadata["owner"] = QString::fromStdString(baseUi->metadataContainer.owner);
        metadata["type"] = "raw";
        metadata["ownerGroup"] = QString::fromStdString(baseUi->metadataContainer.ownerGroup);
        metadata["creationTime"] = baseUi->ui->dateTimeEdit->dateTime().toString(Qt::ISODate);
        metadata["investigator"] = baseUi->metadataContainer.investigator;
        auto dsmodel = baseUi->ui->tableWidget->model();
        QStringList inputDatasetsList;
        for (int ii=0; ii<dsmodel->rowCount(); ii++){
            inputDatasetsList.push_back(dsmodel->data(dsmodel->index(ii,1), Qt::DisplayRole).toString());
        }
        metadata["inputDatasets"] = QJsonArray::fromStringList(inputDatasetsList);
        metadata["usedSoftware"] = baseUi->metadataContainer.usedSoftware;
    }


    QJsonDocument saveDoc(metadata);
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

    QFile filelistingFile(QDir(dirpath).filePath(baseUi->ingestionContainer.filenameFilelisting));
    if (!filelistingFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        baseUi->sendWarning("Could not open filelisting file.");
        return;
    }

    QTextStream out(&filelistingFile);
    for (auto v:baseUi->filelistingContainer){
        out << v << "\n";
    }
    filelistingFile.close();
}

Ingestor::Ingestor(std::string commandIn){
    command = commandIn;
}

void Ingestor::connectToUi(Dialog *uiDialog){
    baseUi = uiDialog;
}

void Ingestor::run(){
    utils::system_call(command);
}



QString extractSourceFolder(std::vector<QString> container){
// a quick and dirty solution... not very efficient though

    QString lPath = "/";
    int ii=1;
    bool entryIsValid = true;
    QString subdir;
    while (ii<container[0].length()){
        for (auto v:container){
            if (v.length()==ii){
                entryIsValid = false;
            }
        }
        if (!entryIsValid){
            break;
        }
        if (container[0][ii]=='/'){
            lPath.push_back(subdir);
            subdir.clear();
        }
        subdir.push_back(container[0][ii]);
        ii++;
    }
    return lPath;

}

QJsonObject treeModel2Json(QTreeWidgetItem *item){
    QJsonObject retObj;
    for (int ii=0; ii<item->childCount(); ii++){
        QJsonObject tmpObject;
        if (item->child(ii)->childCount()>0){
            tmpObject = treeModel2Json(item->child(ii));
            retObj.insert(item->child(ii)->data(0,0).toString(), tmpObject);
        } else {
            retObj.insert(item->child(ii)->data(0,0).toString(), item->child(ii)->data(1,0).toString());
        }
    }

    return retObj;
}




