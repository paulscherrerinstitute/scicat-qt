#include "dialog.h"

extern QVersionNumber currentVersionNumber;

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    ui->setupUi(this);

    // TODO hidden objects where need to be fixed/implemented
    ui->pushButtonDetach->hide();
    ui->pushButtonKill->hide();
    // end

    QPixmap pixmap(":/images/scicat-logo-white.png");
    pixmap = pixmap.scaledToWidth(50);
    ui->labelLogo->setPixmap(pixmap);
    ui->labelLogo->setMask(pixmap.mask());


    // update number of slides/pages
    numSlides = ui->stackedWidget->count();

    //initialize UI
    ui->stackedWidget->setCurrentIndex(0);
    ui->metadataGuided->setCurrentIndex(0);
    ui->stackedWidgetNavigationButtons->setCurrentIndex(0);
    ui->nextSlideButton->hide();
    ui->pushButtonRetrieveData->hide();
    ui->pushButtonArchiveData->hide();
    ui->labelManualEntry->hide();
    ui->groupBoxProposal->hide();
    ui->labelProposal->hide();
    ui->comboBoxCreationLocation->setCurrentIndex(-1);



    QSettings settingsContainer("PSI", "SciCat");

    // flush the settings container if the major version number has changed
    QVersionNumber oldVersion = QVersionNumber::fromString(settingsContainer.value("version", QVersionNumber(-1,0).toString()).toString());
    if (oldVersion.isNull() || (oldVersion.majorVersion()!=currentVersionNumber.majorVersion())){
        qDebug() << "Current version number has changed. Flushing settings container.";
        settingsContainer.clear();
    }

    updateSettingsContainer(settingsContainer, "version", currentVersionNumber.toString());
    updateSettingsContainer(settingsContainer, "investigatorIsPI", defaultSettings.investigatorIsPI);
    updateSettingsContainer(settingsContainer, "binaryPath", defaultSettings.binaryPath);
    updateSettingsContainer(settingsContainer, "datasetRetriever", defaultSettings.datasetRetriever);
    updateSettingsContainer(settingsContainer, "logPath", defaultSettings.logPath);
    updateSettingsContainer(settingsContainer, "filenameFilelisting", defaultSettings.filenameFilelisting);
    updateSettingsContainer(settingsContainer, "filenameMetadata", defaultSettings.filenameMetadata);
    updateSettingsContainer(settingsContainer, "deleteLog", defaultSettings.deleteLog);
    updateSettingsContainer(settingsContainer, "serverID", defaultSettings.serverID);
    updateSettingsContainer(settingsContainer, "printIngestion", defaultSettings.printIngestion);
    updateSettingsContainer(settingsContainer, "disableSystemCalls", defaultSettings.disableSystemCalls);
    updateSettingsContainer(settingsContainer, "usePgroupAsDefaultPath", defaultSettings.usePgroupAsDefaultPath);
    updateSettingsContainer(settingsContainer, "saveProposalInfo", defaultSettings.saveProposalInfo);
    updateSettingsContainer(settingsContainer, "loadProposalInfo", defaultSettings.loadProposalInfo);
    updateSettingsContainer(settingsContainer, "enableArchive", defaultSettings.enableArchive);
    updateSettingsContainer(settingsContainer, "enableIngest", defaultSettings.enableIngest);
    updateSettingsContainer(settingsContainer, "disableEmails", defaultSettings.disableEmails);
    updateSettingsContainer(settingsContainer, "saveLog", defaultSettings.saveLog);
    updateSettingsContainer(settingsContainer, "requestAlreadyRetrievedData", defaultSettings.requestAlreadyRetrievedData);
    updateSettingsContainer(settingsContainer, "guessDatasetname", defaultSettings.guessDatasetname);




    // connect button signals to slots
    connect(ui->nextSlideButton, SIGNAL (clicked()), this, SLOT (nextSlide()));
    connect(ui->addFilesDirectoriesButton, SIGNAL (clicked()), this, SLOT (fileDialogFilelisting()));
    connect(ui->clearButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearFilelisting()));
    connect(ui->clearAllButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearAllFilelisting()));
    connect(ui->clearButtonDatasets, SIGNAL (clicked()), this, SLOT (clearDatasets()));
    connect(ui->clearAllButtonDatasets, SIGNAL (clicked()), this, SLOT (clearAllDatasets()));
    connect(ui->lineEditPgroup, SIGNAL (editingFinished()), this, SLOT (getPI()));
    connect(ui->pushButtonGoToPgroup, SIGNAL (clicked()), this, SLOT (goToPgroup()));
    connect(ui->pushButtonNavigationFiles, SIGNAL (clicked()), this, SLOT (goToFiles()));
    connect(ui->pushButtonNavigationMetadata, SIGNAL (clicked()), this, SLOT (goToMetadata()));
    connect(ui->pushButtonNavigationSubmit, SIGNAL (clicked()), this, SLOT (goToSubmit()));
    connect(ui->pushButtonAddDatasets, SIGNAL (clicked()), this, SLOT (openDatasetBrowser()));
    connect(ui->comboBoxDatatype, SIGNAL (currentIndexChanged(int)), this, SLOT (comboBoxDatatypeChanged(int)));
    connect(ui->toolButtonPlus, SIGNAL(clicked()), this, SLOT(appendMetadataEntry()));
    connect(ui->toolButtonMinus, SIGNAL(clicked()), this, SLOT(removeMetadataEntry()));
    connect(ui->lineEditPgroup, SIGNAL(editingFinished()), this, SLOT(piChanged()));
    connect(ui->lineEditPIMail, SIGNAL(editingFinished()), this, SLOT(piChanged()));
    connect(ui->lineEditPIFirstname, SIGNAL(editingFinished()), this, SLOT(piChanged()));
    connect(ui->lineEditPILastname, SIGNAL(editingFinished()), this, SLOT(piChanged()));
    connect(ui->pushButtonRestart, SIGNAL(clicked()), this, SLOT(restart()));
    connect(ui->lineEditDatasetName, SIGNAL(editingFinished()), this, SLOT(datasetNameCheck()));
    connect(ui->lineEditInvestigator, SIGNAL(editingFinished()), this, SLOT(investigatorChanged()));
    connect(ui->textEditDescription, SIGNAL(textChanged()), this, SLOT(descriptionChanged()));
    connect(ui->textEditUsedSoftware, SIGNAL(textChanged()), this, SLOT(usedSoftwareChanged()));
    connect(ui->pushButtonGoToArchive, SIGNAL(clicked()), this, SLOT(goToArchive()));
    connect(ui->pushButtonGoToRetrieve, SIGNAL(clicked()), this, SLOT(goToRetrieve()));
    connect(ui->pushButtonExportMetadata, SIGNAL(clicked()), this, SLOT(exportMetadata()));
    connect(ui->pushButtonImportMetadata, SIGNAL(clicked()), this, SLOT(importMetadata()));
    connect(ui->pushButtonArchiveData, SIGNAL(clicked()), this, SLOT(goToArchive()));
    connect(ui->pushButtonRetrieveData, SIGNAL(clicked()), this, SLOT(goToRetrieve()));
    connect(ui->pushButtonRetrieve, SIGNAL(clicked()), this, SLOT(requestRetrieve()));
    connect(ui->pushButtonRequest, SIGNAL(clicked()), this, SLOT(goToRequest()));
    connect(ui->pushButtonJobs, SIGNAL(clicked()), this, SLOT(goToJobs()));
    connect(ui->pushButtonKill, SIGNAL(clicked()), this, SLOT(killJob()));
    connect(ui->pushButtonDetach, SIGNAL(clicked()), this, SLOT(detachJob()));
    connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveRetrievedData()));
    connect(ui->pushButtonSettings, SIGNAL(clicked()), this, SLOT(openSettings()));
    connect(ui->pushButtonOpenLog, SIGNAL(clicked()), this, SLOT(openIngestionLog()));
    connect(ui->comboBoxCreationLocation, SIGNAL(currentIndexChanged(int)), this, SLOT(creationLocationChanged(int)));
    connect(ui->pushButtonRefreshJobs, SIGNAL(clicked()), this, SLOT(updateJobTable()));



    // warning label
    ui->warningLabel->setVisible(false);

    // prepare function handles for nextSlideButton
    nextSlideButtonFunctions.push_back(&Dialog::PINextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::filelistingNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::metadataNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::quit);


    // pgroup page
    ui->lineEditPIMail->setEnabled(false);
    ui->lineEditPIFirstname->setEnabled(false);
    ui->lineEditPILastname->setEnabled(false);
    ui->lineEditProposalID_2->setEnabled(false);


    // navigation buttons
    navigationButtons.push_back(ui->pushButtonGoToPgroup);
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

    ui->comboBoxCreationLocation->addItems(defaultSettings.creationLocationList);
    ui->comboBoxCreationLocation->hide();
    ui->labelCreationLocation->hide();


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

    ui->waitingCircle->setText("Dummy");
    if (!movie->isValid())
        return;

    ui->pushButtonGoToPgroup->setIcon(QIcon(":/images/login.svg"));
    ui->pushButtonGoToPgroup->setIconSize(QSize(50,50));

    loginIcons.push_back(":/images/login.svg");
    loginIcons.push_back(":/images/login-active.svg");
    loginIcons.push_back(":/images/login-not-active.svg");

    archiveIcons.push_back(":/images/archive.svg");
    archiveIcons.push_back(":/images/archive-active.svg");
    archiveIcons.push_back(":/images/archive-not-active.svg");

    retrieveIcons.push_back(":/images/retrieve.svg");
    retrieveIcons.push_back(":/images/retrieve-active.svg");
    retrieveIcons.push_back(":/images/retrieve-not-active.svg");

    pgroupWatcher->setIcons(loginIcons);
    archiveWatcher->setIcons(archiveIcons);
    retrieveWatcher->setIcons(retrieveIcons);

    ui->pushButtonGoToPgroup->installEventFilter(pgroupWatcher);
    ui->pushButtonGoToArchive->installEventFilter(archiveWatcher);
    ui->pushButtonGoToRetrieve->installEventFilter(retrieveWatcher);


    // dataset retriever
    ui->tableWidgetRetriever->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetRetriever->insertColumn(0);
    ui->tableWidgetRetriever->insertColumn(1);
    ui->tableWidgetRetriever->insertColumn(2);
    ui->tableWidgetRetriever->insertColumn(3);
    ui->tableWidgetRetriever->setColumnWidth(0, 200);
    ui->tableWidgetRetriever->setColumnWidth(1, 100);
    ui->tableWidgetRetriever->setColumnWidth(2, 100);
    ui->tableWidgetRetriever->setColumnWidth(3, 300);
    ui->tableWidgetRetriever->setHorizontalHeaderLabels(QStringList({"Name", "Retrievable", "Retrieved", "Dataset ID"}));


    // job viewer
    ui->tableWidgetJobs->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetJobs->insertColumn(0);
    ui->tableWidgetJobs->insertColumn(1);
    ui->tableWidgetJobs->insertColumn(2);
    ui->tableWidgetJobs->insertColumn(3);
    ui->tableWidgetJobs->setColumnWidth(0, 200);
    ui->tableWidgetJobs->setColumnWidth(1, 200);
    ui->tableWidgetJobs->setColumnWidth(2, 200);
    ui->tableWidgetJobs->setColumnWidth(3, 300);
    ui->tableWidgetJobs->setHorizontalHeaderLabels(QStringList({"Name", "Path", "Process ID", "Dataset ID"}));

    ingestionLog = new LogBrowser(this);

    styleSheetsContainer.buttonNavigation = "QPushButton{background-color:rgb(0, 45, 59); border-radius:0px}; "
                                          "QPushButton:hover{background-color:rgb(0, 70, 91); border-radius:0px}; "
                                          "QPushButton:!enabled{background-color:rgb(0, 45, 59); border-radius:0px; color:rgb(198, 198, 198)}";

}

void Dialog::sendWarning(QString warningMsg){
    ui->warningLabel->setText(warningMsg);
    ui->warningLabel->setVisible(true);
    QTimer::singleShot(5000, ui->warningLabel, &QLabel::hide);
}

void Dialog::toggleNavigationBar(int section){
    if (secID==section){
        // already there; just toggle the navigation buttons
        ui->stackedWidgetNavigationButtons->setVisible(!(ui->stackedWidgetNavigationButtons->isVisible()));
    } else {
        ui->stackedWidgetNavigationButtons->setCurrentIndex(section);
        secID=section;
    }
}

void Dialog::updateNavigationIcons(int targetID){
    if (secID!=targetID){

        switch (secID) {
        case 0:{
            ui->pushButtonGoToPgroup->setIcon(QIcon(":/images/login-not-active.svg"));
            ui->pushButtonGoToPgroup->setIconSize(QSize(50,50));
            pgroupWatcher->setButtonIsActive(false);
            break;
        }

        case 1: {
            ui->pushButtonGoToArchive->setIcon(QIcon(":/images/archive-not-active.svg"));
            ui->pushButtonGoToArchive->setIconSize(QSize(50,50));
            archiveWatcher->setButtonIsActive(false);
            break;
        }

        case 2:{
            ui->pushButtonGoToRetrieve->setIcon(QIcon(":/images/retrieve-not-active.svg"));
            ui->pushButtonGoToRetrieve->setIconSize(QSize(50,50));
            retrieveWatcher->setButtonIsActive(false);
            break;
        }

        }
        switch (targetID) {
        case 0:{
            ui->pushButtonGoToPgroup->setIcon(QIcon(":/images/login-active.svg"));
            ui->pushButtonGoToPgroup->setIconSize(QSize(50,50));
            pgroupWatcher->setButtonIsActive(true);
            break;
        }
        case 1: {
            ui->pushButtonGoToArchive->setIcon(QIcon(":/images/archive-active.svg"));
            ui->pushButtonGoToArchive->setIconSize(QSize(50,50));
            archiveWatcher->setButtonIsActive(true);
            break;
        }
        case 2: {
            ui->pushButtonGoToRetrieve->setIcon(QIcon(":/images/retrieve-active.svg"));
            ui->pushButtonGoToRetrieve->setIconSize(QSize(50,50));
            retrieveWatcher->setButtonIsActive(true);
            break;
        }



        }

    }
}

void Dialog::goToArchive(){
    if (checkPIInfo()){
        if (secID!=1){
            updateNavigationIcons(1);
        }
        toggleNavigationBar(1);
        if ((ui->stackedWidget->currentIndex()>(numPagesArchive+numPagesLogin-1)) || (ui->stackedWidget->currentIndex()==0)){
                ui->stackedWidget->setCurrentIndex(numPagesLogin);
        }
        ui->nextSlideButton->show();
    }
}

void Dialog::goToRetrieve(){
    if (metadataContainer.isBeamlineAccount){
        QMessageBox::information(this, tr("Permission error"), tr("Retrieving data with beamline accounts is not supported. Please use your personal PSI account instead.") );
        return;
    }
    if (checkPIInfo()){
        if (secID!=2){
            updateNavigationIcons(2);
        }
        toggleNavigationBar(2);
        ui->stackedWidget->setCurrentIndex(numPagesLogin+numPagesArchive);
        ui->nextSlideButton->hide();
        qApp->processEvents();
        updateRetrieveList();


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
    QJsonDocument userInfoReponse = QJsonDocument::fromJson(QString::fromStdString(metadataContainer.userInfo).toUtf8());
    std::vector<QString>finishedSuccessfulPIDs;
    qDebug() << userInfoReponse;
    for (auto v:userInfoReponse.object().find("currentGroups").value().toArray()){
        availPgroups.push_back(v.toString());
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

}

void Dialog::goToPgroup(){
    if (secID!=0){
        updateNavigationIcons(0);
    }
    goToPage(0);
    toggleNavigationBar(0);
    ui->nextSlideButton->hide();

}

void Dialog::goToFiles(){
    if (checkPIInfo()){
        goToPage(numPagesLogin);
        ui->nextSlideButton->setText("Next");
    }
}

void Dialog::goToMetadata(){
    if (checkPIInfo() && filelistingCheckAndUpdate()){
        goToPage(numPagesLogin+1);
        ui->nextSlideButton->setText("Next");
    }
}

void Dialog::goToSubmit(){
    QSettings settingsContainer("PSI", "SciCat");
    if (checkPIInfo() && filelistingCheckAndUpdate() && checkMetadata()){
        goToPage(numPagesLogin+2);
        ui->pushButtonRestart->setText("Submit job");
        ui->waitingMessage->hide();
        ui->nextSlideButton->setText("Done");
        ui->checkBoxIngest->setChecked(settingsContainer.value("enableIngest").toBool());
        ui->checkBoxArchive->setChecked(settingsContainer.value("enableArchive").toBool());
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
    // raw data requires creationLocation
    if (ui->comboBoxDatatype->currentIndex()==1){
        if (ui->comboBoxCreationLocation->currentIndex()==-1){
            QMessageBox::information(this, tr("Invalid entry"), tr("Instrument cannot be empty.") );
            ui->metadataGuided->setCurrentIndex(0);
            return false;
        }
    }
    if (ui->comboBoxDatatype->currentIndex()==0){
        if (ui->lineEditInvestigator->text().isEmpty()){
            QMessageBox::information(this, tr("Invalid entry"), tr("Investigator cannot be empty") );
            ui->metadataGuided->setCurrentIndex(0);
            return false;
        }
    }

    metadataContainer.datasetName = ui->lineEditDatasetName->text();
    return true;
}

void Dialog::creationLocationChanged(int val){
    (void) val;
    if (ui->stackedWidget->currentIndex()==2){
        metadataContainer.creationLocation = ui->comboBoxCreationLocation->currentText();
    }
}

bool Dialog::filelistingCheckAndUpdate(){
    QSettings settingsContainer("PSI", "SciCat");

    if (ui->filelistingListWidget->count()==0){
        QMessageBox::information(this, tr("Invalid entry"), tr("Please add at least one file/directory.") );
        return false;
    }
    // prepare metadata entries
    ui->lineEditOwnerEmail->setText(QString::fromStdString(metadataContainer.ownerEmail));
    ui->lineEditOwner->setText(QString::fromStdString(metadataContainer.owner));
    ui->lineEditOwnerGroup->setText(QString::fromStdString(metadataContainer.ownerGroup));

    if (metadataContainer.proposalID.length()>0){
        ui->lineEditProposalID->setText(QString::fromStdString(metadataContainer.proposalID));
        auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Proposals/" + QString::fromStdString(metadataContainer.proposalID).replace("/", "%2F").toStdString() + "?access_token=" + token + "'");
        metadataContainer.creationLocation = QString::fromStdString(utils::extractFromJson(tmp, "instrument"));
        ui->comboBoxCreationLocation->setCurrentIndex(ui->comboBoxCreationLocation->findText(metadataContainer.creationLocation));
    } else {
        if (metadataContainer.creationLocation.length()==0){
            ui->comboBoxCreationLocation->setCurrentIndex(-1);
        }
    }
    ui->lineEditInvestigator->setText(metadataContainer.investigator);

    filelistingContainer.clear();
    for (int ii=0; ii<ui->filelistingListWidget->count(); ii++ ){
        filelistingContainer.push_back(ui->filelistingListWidget->item(ii)->text());
    }

    // guess dataset name
    if (settingsContainer.value("guessDatasetname", defaultSettings.guessDatasetname).toBool()){
        if (filelistingContainer.size()==1){
            QFileInfo info(filelistingContainer[0]);
            ui->lineEditDatasetName->setText(info.baseName());

        } else if (filelistingContainer.size()>1){
            auto sourceFolder = extractSourceFolder(filelistingContainer);
            QFileInfo info(sourceFolder);
            ui->lineEditDatasetName->setText(info.baseName());
        }
    }

    // count files
    int count = 0;
    for (auto v:filelistingContainer) {
        count += countFilesAndDirRecursive(v);
    }
    if (count>150000){
        QMessageBox::information(this, tr("Warning"), tr("The selected directories exceed the maximum number of files per dataset. Your dataset will be split into smaller datasets.") );
    }
    qDebug() << "Number of files: " << count;


    return true;
}

void Dialog::filelistingNextSlide(){
    goToMetadata();
}
void Dialog::metadataNextSlide(){
    goToSubmit();
}

void Dialog::setButtonsEnabled(bool enabled){
    ui->nextSlideButton->setEnabled(enabled);
    ui->pushButtonGoToPgroup->setEnabled(enabled);
    ui->pushButtonNavigationFiles->setEnabled(enabled);
    ui->pushButtonNavigationMetadata->setEnabled(enabled);
    ui->pushButtonNavigationSubmit->setEnabled(enabled);
    ui->pushButtonGoToArchive->setEnabled(enabled);
    ui->pushButtonGoToRetrieve->setEnabled(enabled);


}

void Dialog::prepareIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    QString dirpath = settingsContainer.value("logPath", defaultSettings.logPath).toString() + "/ingestion/" + metadataContainer.datasetName + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    metadataContainer.ingestionLogDir = dirpath;
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            sendWarning("Could not write to output directory.");
        }
    }

    QString metafname = QDir(dirpath).filePath(settingsContainer.value("filenameMetadata", defaultSettings.filenameMetadata).toString());
    QFile metadataFile(metafname);
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        sendWarning("Could not open metadata file.");
        return;
    }
    metadataContainer.metadataFilename = metafname;

    // prepare metadata file
    QJsonObject metadata;
    bool isRawData = (bool) ui->comboBoxDatatype->currentIndex();
    auto sourceFolder = extractSourceFolder(filelistingContainer);
    metadata["sourceFolder"] = sourceFolder;
    metadata["owner"] = QString::fromStdString(metadataContainer.owner);
    metadata["ownerGroup"] = QString::fromStdString(metadataContainer.ownerGroup);
    metadata["ownerEmail"] = QString::fromStdString(metadataContainer.ownerEmail);
    metadata["description"] = metadataContainer.description;
    metadata["datasetName"] = metadataContainer.datasetName;


    if (isRawData){
        qDebug() << "Preparing metadata file for raw data";
        metadata["principalInvestigator"] = QString::fromStdString(metadataContainer.PIemail); // this should be changed in the future; there is no need to save it as std::string
        metadata["creationLocation"] = metadataContainer.creationLocation;
        metadata["type"] = "raw";
        auto res = ui->treeWidget->findItems("scientificMetadata", Qt::MatchFlag::MatchExactly);
        QJsonArray scMetadata;
        for (int ii=0; ii<res.size(); ii++){
            qDebug() << res[ii]->childCount();
            scMetadata.push_back(treeModel2Json(res[ii]));
        }
        metadata["scientificMetadata"] = scMetadata;
        qDebug() << metadataContainer.datasetName;
    } else {
        qDebug() << "Preparing metadata file for derived data";
        metadata["type"] = "derived";
        metadata["investigator"] = metadataContainer.investigator;
        auto dsmodel = ui->tableWidget->model();
        QStringList inputDatasetsList;
        for (int ii=0; ii<dsmodel->rowCount(); ii++){
            inputDatasetsList.push_back(dsmodel->data(dsmodel->index(ii,1), Qt::DisplayRole).toString());
        }
        if (inputDatasetsList.length()==0){
            metadata["inputDatasets"] = QJsonArray::fromStringList(QStringList("N/A"));
        } else {
            metadata["inputDatasets"] = QJsonArray::fromStringList(inputDatasetsList);
        }
        if (metadataContainer.usedSoftware.length()==0){
            metadata["usedSoftware"] = QJsonArray::fromStringList(QStringList("N/A"));
        } else {
            metadata["usedSoftware"] = QJsonArray::fromStringList(QStringList(metadataContainer.usedSoftware));
        }
    }


    QJsonDocument saveDoc(metadata);
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

    QString filefname = QDir(dirpath).filePath(settingsContainer.value("filenameFilelisting", defaultSettings.filenameFilelisting).toString());
    QFile filelistingFile(filefname);
    if (!filelistingFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        sendWarning("Could not open filelisting file.");
        return;
    }
    metadataContainer.filelistingFilename = filefname;

    QTextStream out(&filelistingFile);
    for (auto v:filelistingContainer){
        QString tmpFilename = v.remove(sourceFolder);
        if (tmpFilename[0]=="/"){
            tmpFilename.remove(0,1);
        }
        out << tmpFilename << "\n";
    }
    filelistingFile.close();
}

void Dialog::startIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    ui->waitingMessage->show();
    ui->pleaseWait->setText("Please wait.");
    ui->waitingCircle->setMovie(movie);
    ui->waitingCircle->show();
    movie->start();

    // disable buttons
    setButtonsEnabled(false);
    ui->pushButtonRestart->setEnabled(false);

    // compile ingestion command
    QStringList arguments;

    if (ui->checkBoxIngest->isChecked()>0){
        arguments << "--ingest";
    }
    if (ui->checkBoxArchive->isChecked()>0){
        arguments << "--autoarchive";
    }
    switch (settingsContainer.value("serverID", defaultSettings.serverID).toInt()){
    case 0:
        break;
    case 1:
        arguments << "-testenv";
        break;
    case 2:
        arguments << "-devenv";
        break;
    }

    arguments << "--noninteractive" << "--allowexistingsource" << "--token" << QString::fromStdString(token) << metadataContainer.metadataFilename << metadataContainer.filelistingFilename;

    // start ingestion
    qDebug() << "starting ingestion";
//    ingestor_m = new Ingestor(cmd);
    ingestion = new QProcess(this);
    ingestion->setProcessChannelMode(QProcess::MergedChannels);
    connect(ingestion, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ingestionFinished(int)));
    connect(ingestion, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ingestionFailed(QProcess::ProcessError)));
    connect(ingestion, SIGNAL(readyReadStandardOutput()), this, SLOT(readIngestionOutput()));

    if (settingsContainer.value("printIngestion", defaultSettings.printIngestion).toBool()){
        qDebug() << settingsContainer.value("binaryPath", defaultSettings.binaryPath).toString() << " " << arguments;
    }
    if (!settingsContainer.value("disableSystemCalls", defaultSettings.disableSystemCalls).toBool()){
        ingestion->start(settingsContainer.value("binaryPath", defaultSettings.binaryPath).toString(), arguments);
//        ingestion->start("ls ./");
    } else {
        ingestion->finished(0, QProcess::NormalExit);
    }



}

void Dialog::ingestionFailed(QProcess::ProcessError error){
    ingestionLog->ui->textEditIngestionLog->append("Error: " + (QString) error);
    ingestionLog->ui->textEditIngestionLog->append(ingestion->errorString());
    qDebug() << "Error: " << error;
    ingestionFinished(1);
}

void Dialog::readIngestionOutput(){
    while(ingestion->canReadLine()){
        ingestionLog->ui->textEditIngestionLog->append(ingestion->readLine());
    }

}

void Dialog::ingestionFinished(int exitCode){
    QSettings settingsContainer("PSI", "SciCat");

    movie->stop();
    ui->waitingCircle->hide();
    ui->nextSlideButton->setEnabled(true);
    ingested = true;
    ui->pushButtonRestart->setEnabled(true);
    ui->pushButtonRestart->setText("Restart");

    if (exitCode==0){
        // ingestion finished, reenable buttons
        ui->pleaseWait->setText("Done.");
    } else {
        ui->pleaseWait->setText("Ingestion failed.");
    }

    if (settingsContainer.value("deleteLog", defaultSettings.deleteLog).toBool()){
        qDebug() << "removing ingestion log files";
//        qDebug() << metadataContainer.ingestionLogDir;
        if (!QDir(metadataContainer.ingestionLogDir).removeRecursively()){
            sendWarning("Failed to remove ingestion log files.");
        }
    } else if (settingsContainer.value("saveLog", defaultSettings.saveLog).toBool()){
        ingestionLog->saveLog(metadataContainer.ingestionLogDir + "/ingestion.log");
    }

}

void Dialog::resetContent(){
    QSettings settingsContainer("PSI", "SciCat");

    setButtonsEnabled(true);
    filelistingContainer.clear();
    clearAllDatasets();
    clearAllFilelisting();
    ui->metadataGuided->setCurrentIndex(0);
    ui->nextSlideButton->setText("Next");
    ingested = false;
    ingestionLog->clear();
}

void Dialog::restart(){
    if (ingested){
        resetContent();
        goToPgroup(); // go back to PI
    } else {
        prepareIngestion();
        startIngestion();
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
    QSettings settingsContainer("PSI", "SciCat");

    pgroupIsValid = true;
    if (ui->lineEditPgroup->text().size()==0){
        QMessageBox::information(this, tr("Error"), tr("pgroup cannot be empty") );
        pgroupIsValid = false;
        return pgroupIsValid;
    } else if ((ui->lineEditPgroup->text().size() != 6) || (ui->lineEditPgroup->text().toStdString().find("p") != 0)){
        QMessageBox::information(this, tr("Error"), tr("Please enter a valid pgroup") );
        pgroupIsValid = false;
        return pgroupIsValid;
    }

    // check if user has access to the given pgroup
    if (!availPgroups.contains(ui->lineEditPgroup->text())){
        auto msg = "Cannot access pgroup " + ui->lineEditPgroup->text().toStdString() + ". Permission denied.";
        QMessageBox::information(this, tr("Error"), tr(msg.c_str()));
        pgroupIsValid = false;
    }

    return pgroupIsValid;
}

bool Dialog::checkPIInfo(){
    QSettings settingsContainer("PSI", "SciCat");

    if ((PIHasChanged) || (!PIIsValid)){
        qDebug() << "checking PI";
        PIHasChanged = false;
        PIIsValid = checkPgroup(); // this should also check the PI info, not only the pgroup

        metadataContainer.PIemail = ui->lineEditPIMail->text().toStdString();
        metadataContainer.PIFirstname = ui->lineEditPIFirstname->text().toStdString();
        metadataContainer.PILastname = ui->lineEditPILastname->text().toStdString();
        metadataContainer.proposalID = ui->lineEditProposalID_2->text().toStdString();


        if (metadataContainer.PIFirstname.length()==0){
            QMessageBox::information(this, tr("Error"), tr("PI name cannot be empty") );
            return false;
        }

        if (metadataContainer.PILastname.length()==0){
            QMessageBox::information(this, tr("Error"), tr("PI name cannot be empty") );
            return false;
        }

        if (metadataContainer.PIemail.length()==0){
            QMessageBox::information(this, tr("Error"), tr("Please enter a valid email address") );
            return false;
        }

        // set defaults for owner/ownergroup etc
        metadataContainer.ownerGroup = metadataContainer.pgroup;
        metadataContainer.owner = metadataContainer.PIFirstname + " " + metadataContainer.PILastname;
        metadataContainer.ownerEmail = metadataContainer.PIemail;
        settingsContainer.value("investigatorIsPI", defaultSettings.investigatorIsPI).toBool() ? metadataContainer.investigator=QString::fromStdString(metadataContainer.PIemail) : metadataContainer.investigator=QString::fromStdString(utils::extractFromJson(metadataContainer.userInfo, "currentUserEmail"));

        // save PI info if needed
        if ((!metadataContainer.retrievedPIInfoSuccessfully) && (settingsContainer.value("saveProposalInfo", defaultSettings.saveProposalInfo).toBool())){
            qDebug() << "Writing PI info";
            QString dirpath = settingsContainer.value("logPath", defaultSettings.logPath).toString() + "/proposals/";
            if (!QDir(dirpath).exists()){
                qDebug()<<"Output directory does not exist";
                if (!QDir().mkpath(dirpath)){
                    sendWarning("Could not write to output directory.");
                    return PIIsValid;
                }
            }

            QString proposalFile = QDir(dirpath).filePath(QString::fromStdString("proposal_" + metadataContainer.pgroup+".dat"));

            QFile file(proposalFile);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
                sendWarning("Could not open proposal file for writing.");
            } else {
                QTextStream out(&file);
                out << QString::fromStdString(metadataContainer.PIFirstname) << "\n";
                out << QString::fromStdString(metadataContainer.PILastname) << "\n";
                out << QString::fromStdString(metadataContainer.PIemail) << "\n";
                file.close();
            }

        }


        return PIIsValid;
    }

    return true;

}

void Dialog::getPI(){
    QSettings settingsContainer("PSI", "SciCat");
    if (Dialog::checkPgroup()){
        metadataContainer.pgroup = ui->lineEditPgroup->text().toStdString();
        auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Proposals?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.pgroup + "%22%7D%7D&access_token=" + token + "'");
        if (tmp.length()>5){
            metadataContainer.PIFirstname = utils::extractFromJson(tmp, "pi_firstname");
            metadataContainer.PILastname = utils::extractFromJson(tmp, "pi_lastname");
            metadataContainer.PIemail = utils::extractFromJson(tmp, "pi_email");
            metadataContainer.proposalTitle = utils::extractFromJson(tmp, "title");
            metadataContainer.proposalAbstract = utils::extractFromJson(tmp, "abstract");
            metadataContainer.instrument = utils::extractFromJson(tmp, "instrument");
            metadataContainer.proposalID = utils::extractFromJson(tmp, "proposalId");

            ui->lineEditPIMail->setText(QString::fromStdString(metadataContainer.PIemail));
            ui->lineEditPIFirstname->setText(QString::fromStdString(metadataContainer.PIFirstname));
            ui->lineEditPILastname->setText(QString::fromStdString(metadataContainer.PILastname));
            ui->lineEditProposalID_2->setText(QString::fromStdString(metadataContainer.proposalID));
            ui->lineEditTitle->setText(QString::fromStdString(metadataContainer.proposalTitle));
            ui->textEditAbstract->setText(QString::fromStdString(metadataContainer.proposalAbstract));

            ui->lineEditPIMail->show();
            ui->lineEditPIFirstname->show();
            ui->lineEditPILastname->show();
            ui->lineEditProposalID_2->show();
            ui->lineEditTitle->show();
            ui->textEditAbstract->show();
            ui->labelTitle->show();
            ui->labelAbstract->show();
            ui->groupBoxProposal->show();
            ui->labelProposal->show();
            ui->labelManualEntry->hide();

            ui->lineEditPIMail->setEnabled(false);
            ui->lineEditPIFirstname->setEnabled(false);
            ui->lineEditPILastname->setEnabled(false);
            ui->lineEditProposalID_2->setEnabled(false);
            ui->lineEditTitle->setEnabled(false);
            ui->textEditAbstract->setEnabled(false);

            metadataContainer.retrievedPIInfoSuccessfully = true;


        } else {

            metadataContainer.retrievedPIInfoSuccessfully = false;
            bool manualEdit = false;

            QString dirpath = settingsContainer.value("logPath", defaultSettings.logPath).toString() + "/proposals/";
            QString proposalFile = QDir(dirpath).filePath(QString::fromStdString("proposal_" + metadataContainer.pgroup+".dat"));

            if (settingsContainer.value("loadProposalInfo",defaultSettings.loadProposalInfo).toBool()){
                QFile file(proposalFile);
                if(!file.open(QIODevice::ReadOnly)) {
                    manualEdit = true;
                } else {
                    QTextStream in(&file);

                    QStringList proposalContent;
                    while(!in.atEnd()) {
                        proposalContent.push_back(in.readLine());
                    }
                    qDebug() << proposalContent.size();
                    if (proposalContent.size()<3){
                        manualEdit = true;
                    } else {
                        ui->labelProposal->hide();
                        ui->groupBoxProposal->show();
                        ui->labelManualEntry->hide();
                        ui->lineEditProposalID_2->hide();
                        ui->lineEditTitle->hide();
                        ui->textEditAbstract->hide();
                        ui->labelTitle->hide();
                        ui->labelAbstract->hide();

                        ui->lineEditPIFirstname->setText(proposalContent[0]);
                        ui->lineEditPILastname->setText(proposalContent[1]);
                        ui->lineEditPIMail->setText(proposalContent[2]);
                        metadataContainer.proposalID = "";
                        ui->lineEditProposalID_2->setText(QString::fromStdString(metadataContainer.proposalID));

                        ui->lineEditPIMail->setEnabled(true);
                        ui->lineEditPIFirstname->setEnabled(true);
                        ui->lineEditPILastname->setEnabled(true);
                    }


                    file.close();
                }

            }
           if (manualEdit) {
                ui->labelProposal->hide();
                ui->groupBoxProposal->show();
                ui->labelManualEntry->show();
                ui->lineEditProposalID_2->hide();
                ui->lineEditTitle->hide();
                ui->textEditAbstract->hide();
                ui->labelTitle->hide();
                ui->labelAbstract->hide();

                ui->lineEditPIMail->setText("");
                ui->lineEditPIFirstname->setText("");
                ui->lineEditPILastname->setText("");
                metadataContainer.proposalID = "";
                ui->lineEditProposalID_2->setText(QString::fromStdString(metadataContainer.proposalID));

                ui->lineEditPIMail->setEnabled(true);
                ui->lineEditPIFirstname->setEnabled(true);
                ui->lineEditPILastname->setEnabled(true);
            }

        }

        ui->pushButtonArchiveData->show();
        ui->pushButtonRetrieveData->show();


    }
}

void Dialog::fileDialogFilelisting(){
    fileBrowser fb(this);
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
    QSettings settingsContainer("PSI", "SciCat");
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%20%22" + metadataContainer.pgroup + "%22%7D%2C%20%22fields%22%3A%20%7B%22datasetName%22%3Atrue%2C%20%22pid%22%3Atrue%7D%7D&access_token=" + token + "'");
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

void Dialog::exportMetadata(){
    auto fileName = QFileDialog::getSaveFileName(this,tr("Export scientific metadata"), QDir(QDir::homePath()).filePath(QString::fromStdString(metadataContainer.pgroup + "_scientificMetadata.json")), tr("json files (*.json)"));
    auto res = ui->treeWidget->findItems("scientificMetadata", Qt::MatchFlag::MatchExactly);
    QJsonArray scMetadata;
    for (int ii=0; ii<res.size(); ii++){
        qDebug() << res[ii]->childCount();
        scMetadata.push_back(treeModel2Json(res[ii]));
    }
    QJsonObject metadata;
    metadata["scientificMetadata"] = scMetadata;
    QJsonDocument saveDoc(metadata);
    QFile metadataFile(fileName);
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        sendWarning("Could not export metadata file.");
        return;
    }
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

}


void Dialog::importMetadata(){

    auto fileName = QFileDialog::getOpenFileName(this,tr("Import scientific metadata"), QDir::homePath(), tr("json files (*.json)"));
    QFile importFile(fileName);
    if (!importFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        sendWarning("Could not open metadata file.");
        return;
    }
    QString val = importFile.readAll();
    importFile.close();


    QJsonDocument importJsonFile = QJsonDocument::fromJson(val.toUtf8());
    auto jsonObj = importJsonFile.object();
    ui->treeWidget->clear();
//    addEntryMetadataTreeView("scientificMetadata", "", nullptr, false, 2);
//    auto res = ui->treeWidget->findItems("scientificMetadata", Qt::MatchExactly|Qt::MatchRecursive, 0);
    json2TreeModel(jsonObj, nullptr, ui->treeWidget);
}

void Dialog::comboBoxDatatypeChanged(int val){
//        ui->lineEditInvestigator->setEnabled(!(bool)val);
//        ui->textEditUsedSoftware->setEnabled(!(bool)val);

        // hide input datasets tab if not needed:
        if ((bool)val){
            // raw data

            // store tab widget in container and remove it
            inputDatasetsContainer.tab = ui->metadataGuided->widget(1);
            inputDatasetsContainer.tabLabel = ui->metadataGuided->tabText(1);
            ui->metadataGuided->removeTab(1);

            ui->comboBoxCreationLocation->show();
            ui->labelCreationLocation->show();

            ui->textEditUsedSoftware->hide();
            ui->labelUsedSoftware->hide();
            ui->lineEditInvestigator->hide();
            ui->labelInvestigator->hide();

        } else {
            //derived data
            ui->metadataGuided->insertTab(1,inputDatasetsContainer.tab, inputDatasetsContainer.tabLabel);

            ui->comboBoxCreationLocation->hide();
            ui->labelCreationLocation->hide();

            ui->textEditUsedSoftware->show();
            ui->labelUsedSoftware->show();
            ui->lineEditInvestigator->show();
            ui->labelInvestigator->show();

        }

}

void Dialog::appendMetadataEntry(){
    // check if item supports children
    if (ui->treeWidget->currentItem() != nullptr) {
        if (ui->treeWidget->selectedItems().size()>0){
            if (ui->treeWidget->currentItem()->flags().testFlag(Qt::ItemIsDropEnabled)){
                ui->treeWidget->currentItem()->setData(1,0,"");
                addEntryMetadataTreeView("name", "value", ui->treeWidget->currentItem(), true, 4);

            } else {
                sendWarning("Cannot append child entry to protected field.");
            }
        }
    } else {
        addEntryMetadataTreeView("name", "value", "scientificMetadata", true, 4);
    }
}

void Dialog::removeMetadataEntry(){
    // check if item is protected
    if (ui->treeWidget->currentItem() != nullptr) {
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
    QSettings settingsContainer("PSI", "SciCat");
    auto currentDatasets = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Datasets?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.ownerGroup + "%22%7D%2C%22fields%22%3A%7B%22datasetlifecycle%22%3Atrue%2C%20%22pid%22%3Atrue%2C%20%22datasetName%22%3Atrue%7D%7D&access_token=" + token + "'");
    auto successfulJobs = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "Jobs?filter=%7B%22where%22%3A%20%7B%22and%22%3A%5B%7B%22type%22%3A%22retrieve%22%7D%2C%20%7B%22emailJobInitiator%22%3A%20%22" + metadataContainer.userMail.replace("@", "%40").toStdString() + "%22%7D%5D%7D%2C%22fields%22%3A%7B%22datasetList%22%3Atrue%2C%20%22jobStatusMessage%22%3Atrue%7D%7D&access_token=" + token + "'");
//    qDebug() << "curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "Jobs?filter=%7B%22where%22%3A%20%7B%22and%22%3A%5B%7B%22type%22%3A%22retrieve%22%7D%2C%20%7B%22emailJobInitiator%22%3A%20%22" + metadataContainer.userMail.replace("@", "%40").toStdString() + "%22%7D%5D%7D%2C%22fields%22%3A%7B%22datasetList%22%3Atrue%2C%20%22jobStatusMessage%22%3Atrue%7D%7D&access_token=" + token + "'";
//    qDebug() << successfulJobs;

    // get all 'finishedSuccessful' jobs
    // sadly, there seems to be no option to filter the jobs by pgroup, so we have to query all and select the ones from the current account
    QJsonDocument successfulJobsJson = QJsonDocument::fromJson(QString::fromStdString(successfulJobs).toUtf8());
    std::vector<QString>finishedSuccessfulPIDs;
    for (auto v:successfulJobsJson.array()){
        if (v.toObject().find("jobStatusMessage").value().toString().compare("finishedSuccessful")==0){
            auto datasetListArray = v.toObject().take("datasetList").toArray();
//            qDebug() << v;
            for (int ii=0; ii<datasetListArray.size(); ii++) {
                finishedSuccessfulPIDs.push_back(datasetListArray.at(ii).toObject().find("pid").value().toString());
                qDebug() << datasetListArray.at(ii).toObject().find("pid").value().toString();
            }
        }
    }

    QJsonDocument currentDatasetsJson = QJsonDocument::fromJson(QString::fromStdString(currentDatasets).toUtf8());
    retrieveContainer.clear();
    for (auto v : currentDatasetsJson.array()){
        retrieveContainer_t tmp;
//        qDebug() << v.toObject().find("pid").value().toString();
        tmp.pid = v.toObject().find("pid").value().toString();
        tmp.name = v.toObject().find("datasetName").value().toString();
        bool retrievableBool = v.toObject().find("datasetlifecycle").value().toObject().find("retrievable").value().toBool();
        retrievableBool ? tmp.retrievable="true" : tmp.retrievable="false";
        if (std::find(finishedSuccessfulPIDs.begin(), finishedSuccessfulPIDs.end(), tmp.pid) != finishedSuccessfulPIDs.end()) {
          tmp.retrieved = "true";
        }
        retrieveContainer.push_back(tmp);
    }


    ui->tableWidgetRetriever->setRowCount(0);
    ui->tableWidgetRetriever->clearContents();
    ui->tableWidgetRetriever->setSortingEnabled(false);
    for (auto v : retrieveContainer) {
        ui->tableWidgetRetriever->insertRow(0);
        QTableWidgetItem *tmpItemDataset = new QTableWidgetItem(v.name);
        ui->tableWidgetRetriever->setItem(0,0,tmpItemDataset);
        QTableWidgetItem *tmpItemRetrievable = new QTableWidgetItem(v.retrievable);
        ui->tableWidgetRetriever->setItem(0,1,tmpItemRetrievable);
        QTableWidgetItem *tmpItemStatus = new QTableWidgetItem(v.retrieved);
        ui->tableWidgetRetriever->setItem(0,2,tmpItemStatus);
        QTableWidgetItem *tmpItemPID = new QTableWidgetItem(v.pid);
        ui->tableWidgetRetriever->setItem(0,3,tmpItemPID);
    }
    ui->tableWidgetRetriever->setSortingEnabled(true);

}

void Dialog::openSettings(){
    Settings sett(this);
    sett.exec();
}

void Dialog::openIngestionLog(){
    ingestionLog->exec();
}

void Dialog::quit(){
    close();
}

void Dialog::requestRetrieve(){
    QSettings settingsContainer("PSI", "SciCat");
    QJsonArray datasetList;

    QJsonObject request;
    request["emailJobInitiator"] = metadataContainer.userMail;
    request["type"] = "retrieve";
    {
        QJsonObject jobParams;
        jobParams["username"] = metadataContainer.username;
        jobParams["destinationPath"] = "/archive/retrieve";
        request["jobParams"] = jobParams;
    }
    {
        QModelIndexList indexList = ui->tableWidgetRetriever->selectionModel()->selectedRows();
        for (int ii=0; ii<indexList.size(); ++ii) {
            qDebug() << ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0);
            if (ui->tableWidgetRetriever->item(indexList[ii].row(),1)->data(0).toBool()) { // check retrievable
                qDebug() << (!ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool());
                qDebug() << (settingsContainer.value("requestAlreadyRetrievedData").toBool());
                    if ((!ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool()) || (settingsContainer.value("requestAlreadyRetrievedData").toBool())){
                        QJsonObject tmpDatasetObject;
                        tmpDatasetObject["pid"] = ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0).toString();
                        tmpDatasetObject["file"] = QJsonArray();
                        datasetList.push_back(tmpDatasetObject);
                    } else {
                        sendWarning("Some datasets have already been retrieved and will be skipped.");
                    }
            } else {
                sendWarning("Some datasets are not in a retrievable state.");
            }
        }

        request["datasetList"] = datasetList;
    }
    if (datasetList.size()>0){
        std::string cmd = "curl -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' -d '" + QString(QJsonDocument(request).toJson(QJsonDocument::Compact)).toStdString() +  "' 'https://dacat-qa.psi.ch/api/v3/Jobs?access_token=" + token + "'";
        auto successfulJobs = utils::system_call(cmd);
        qDebug().noquote() << cmd;
        QMessageBox::information(this, tr("Retrieve requested"), tr("The retrieval of your data has been requested. You will be notified via email once the job finished successfully. Successfully retrieved datasets can then be copied using the SciCat Archiver.") );
    }


}

void Dialog::saveRetrievedData(){

    QString dirname = QFileDialog::getExistingDirectory(this, "Save path",QDir::homePath(),QFileDialog::ShowDirsOnly);
    if (dirname.length()>0){
        QModelIndexList indexList = ui->tableWidgetRetriever->selectionModel()->selectedRows();
        for (int ii=0; ii<indexList.size(); ++ii) {
            qDebug() << ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0);
            if (ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool()) { // check retrieve status
                startRetriever(ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0).toString(), dirname, ui->tableWidgetRetriever->item(indexList[ii].row(),0)->data(0).toString());
            } else {
                sendWarning("Some datasets have not been retrieved yet.");
            }
        }
    }

}

void Dialog::startRetriever(const QString &pid, const QString &outdir, const QString &name){
    QSettings settingsContainer("PSI", "SciCat");

    QProcess *retriever = new QProcess(this);
//    retriever->setProcessChannelMode(QProcess::MergedChannels);
    connect(retriever, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(retrieverFinished(int)));
    connect(retriever, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(retrieverFailed(QProcess::ProcessError)));
//    connect(retriever, SIGNAL(readyReadStandardOutput()), this, SLOT(readRetrieverOutput()));

    QStringList arguments;

    switch (settingsContainer.value("serverID", defaultSettings.serverID).toInt()){
    case 0:
        break;
    case 1:
        arguments << "-testenv";
        break;
    case 2:
        arguments << "-devenv";
        break;
    }

    arguments << "-dataset" << pid << "-token" << QString::fromStdString(token) << "-retrieve" << outdir;
    retriever->start(settingsContainer.value("datasetRetriever", defaultSettings.datasetRetriever).toString(), arguments);
    retriever->closeWriteChannel();
    qDebug() << arguments;

    retrieveJobsContainer_t job;
    job.pid = pid;
    job.outdir = outdir;
    job.name = name;
    job.process = retriever;

    retrieverList.push_back(job);

//    // add dummy process for test purposes
//    {
//    QProcess *dummy = new QProcess(this);
//    QStringList dummyArgs;
//    dummyArgs << "30";
//    dummy->start("sleep", dummyArgs);
//    retrieveJobsContainer_t dummyjob;
//    dummyjob.pid = pid;
//    dummyjob.outdir = outdir;
//    dummyjob.name = "sleep 30-1";
//    dummyjob.process = dummy;
//    retrieverList.push_back(dummyjob);
//    }
//    {
//    QProcess *dummy = new QProcess(this);
//    QStringList dummyArgs;
//    dummyArgs << "30";
//    dummy->start("sleep", dummyArgs);
//    retrieveJobsContainer_t dummyjob;
//    dummyjob.pid = pid;
//    dummyjob.outdir = outdir;
//    dummyjob.name = "sleep 30-2";
//    dummyjob.process = dummy;
//    retrieverList.push_back(dummyjob);
//    }
}

void Dialog::retrieverFinished(int val){
    qDebug() << "Retriever finished with exit code " << val;
}

void Dialog::retrieverFailed(QProcess::ProcessError error){
    (void) error;
    sendWarning("Retriever failed.");

}

void Dialog::goToRequest(){
    goToPage(4);
}

void Dialog::goToJobs(){
    goToPage(5);
    updateJobTable();
}

void Dialog::updateRetrieveJobs(){
    // delete all stale PIDs
    for (std::vector<retrieveJobsContainer_t>::iterator it=retrieverList.begin(); it!=retrieverList.end();){
        if ((*it).process->processId()==0){
            it = retrieverList.erase(it);
        } else {
            ++it;
        }

    }
}

void Dialog::updateJobTable(){
    updateRetrieveJobs();
    ui->tableWidgetJobs->setRowCount(0);
    ui->tableWidgetJobs->clearContents();
    for (auto v: retrieverList){
        ui->tableWidgetJobs->insertRow(0);
        qDebug() << v.name;
        qDebug() << v.process->processId();
        QTableWidgetItem *tmpJobName = new QTableWidgetItem(v.name);
        ui->tableWidgetJobs->setItem(0,0,tmpJobName);
        QTableWidgetItem *tmpJobPath = new QTableWidgetItem(v.outdir);
        ui->tableWidgetJobs->setItem(0,1,tmpJobPath);
        QTableWidgetItem *tmpJobPID = new QTableWidgetItem(QString::number(v.process->processId()));
        ui->tableWidgetJobs->setItem(0,2,tmpJobPID);
        QTableWidgetItem *tmpJobID = new QTableWidgetItem(v.pid);
        ui->tableWidgetJobs->setItem(0,3,tmpJobID);
    }

}

void Dialog::killJob(){

}

void Dialog::detachJob(){

}

void Dialog::reject(){
    updateRetrieveJobs();
    if (retrieverList.size()>0){
        auto reply = QMessageBox::question(this, "Quit", "Some retrieve processes haven't finished yet. Do you really want to quit now and abort these processes?");
        if (reply == QMessageBox::Yes) {
          QDialog::reject();
          return;
        } else {
            return;
        }
    } else if (ingestion->state()==QProcess::Running){
        auto reply = QMessageBox::question(this, "Quit", "The ingestion job is still running. Do you really want to quit now and abort the ingestion?");
        if (reply == QMessageBox::Yes) {
          QDialog::reject();
          return;
        } else {
            return;
        }
    }
    QDialog::reject();
}

Dialog::~Dialog()
{
    delete datasets_m;
    delete ui;
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


void json2TreeModel(QJsonObject &inObj, QTreeWidgetItem *item, QTreeWidget* parentWidget){
    qDebug() << "entry: " << inObj;
    auto entryKeys = inObj.keys();
    for (int kk=0;kk<entryKeys.size(); kk++){
        qDebug() << "Creating new item for " << entryKeys[kk];
        QTreeWidgetItem * treeItem;
        if (parentWidget!=nullptr){
            treeItem = new QTreeWidgetItem(parentWidget);
        } else {
            treeItem = new QTreeWidgetItem();
        }
        treeItem->setText(0,entryKeys[kk]);
        auto entryVal = inObj.value(entryKeys[kk]);
        if (entryVal.isArray()){
            qDebug() << "Array: " << entryVal;
            for (int ii=0; ii<entryVal.toArray().size(); ii++){
                auto tmpArrayEntry = entryVal.toArray().at(ii).toObject();
                qDebug() << "calling rec. for " << tmpArrayEntry;
                json2TreeModel(tmpArrayEntry, treeItem);
            }
        } else if (entryVal.isObject()){
            qDebug() << "Object: " << entryVal;
            auto tmpObject = entryVal.toObject();
            json2TreeModel(tmpObject, treeItem);
        } else {
            qDebug() << "Value: " << entryVal;
            treeItem->setText(1,entryVal.toString());

        }
        if (item!=nullptr){
            item->addChild(treeItem);
        }
    }
}

ButtonHoverWatcher::ButtonHoverWatcher(QObject * parent) :
    QObject(parent)
{}

bool ButtonHoverWatcher::eventFilter(QObject * watched, QEvent * event)
{
    QPushButton * button = qobject_cast<QPushButton*>(watched);
    if (!button) {
        return false;
    }

    if (!buttonIsActive){
        if (event->type() == QEvent::Enter) {
            // The push button is hovered by mouse
            button->setIcon(iconsVector[0]);
            button->setIconSize(QSize(50,50));
            return true;
        }

        if (event->type() == QEvent::Leave){
            // The push button is not hovered by mouse
            button->setIcon(iconsVector[2]);
            button->setIconSize(QSize(50,50));
            return true;
        }

    } else {
        button->setIcon(iconsVector[1]);
        button->setIconSize(QSize(50,50));
    }


    return false;
}

void ButtonHoverWatcher::setIcons(std::vector<std::string> icons){
    iconsVector.clear();
    for (uint ii=0; ii<icons.size(); ii++){
        iconsVector.push_back(QIcon(icons[ii].c_str()));
    }

}

void ButtonHoverWatcher::setButtonIsActive(bool val){
    buttonIsActive = val;
}



int countFilesAndDirRecursive(QString dirName){
    QDir dir(dirName);
    int count = 0;
    QStringList fileNameList = dir.entryList();
    qDebug() << fileNameList;
    for (auto v:fileNameList){
        if (!(v.compare(".") || v.compare(".."))){ // ignore .. and .
            QFileInfo info(dirName + "/" + v);
            if (info.isDir()){
                count += countFilesAndDirRecursive(dirName + "/" + v);
            } else {
                ++count;
            }
        }
    }
    return count;
}
