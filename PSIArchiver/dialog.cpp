#include "dialog.h"
#include "./ui_dialog.h"
#include "ui_filebrowser.h"
#include "qfiledialog.h"
#include "QMessageBox"
#include "ui_login.h"
#include "ui_datasetbrowser.h"
#include "ui_settings.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    ui->setupUi(this);

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

    QSettings settingsContainer("PSI", "SciCat");
    updateSettingsContainer(settingsContainer, "investigatorIsPI", defaultSettings.investigatorIsPI);
    updateSettingsContainer(settingsContainer, "binaryPath", defaultSettings.binaryPath);
    updateSettingsContainer(settingsContainer, "logPath", defaultSettings.logPath);
    updateSettingsContainer(settingsContainer, "filenameFilelisting", defaultSettings.filenameFilelisting);
    updateSettingsContainer(settingsContainer, "filenameMetadata", defaultSettings.filenameMetadata);
    updateSettingsContainer(settingsContainer, "deleteLog", defaultSettings.deleteLog);
    updateSettingsContainer(settingsContainer, "serverID", defaultSettings.serverID);
    updateSettingsContainer(settingsContainer, "printIngestion", defaultSettings.printIngestion);
    updateSettingsContainer(settingsContainer, "disableSystemCalls", defaultSettings.disableSystemCalls);




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
    connect(ui->pushButtonExportMetadata, SIGNAL(clicked()), this, SLOT(exportMetadata()));
    connect(ui->pushButtonImportMetadata, SIGNAL(clicked()), this, SLOT(importMetadata()));
    connect(ui->pushButtonArchiveData, SIGNAL(clicked()), this, SLOT(goToArchive()));
    connect(ui->pushButtonRetrieveData, SIGNAL(clicked()), this, SLOT(goToRetrieve()));
    connect(ui->pushButtonSettings, SIGNAL(clicked()), this, SLOT(openSettings()));

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

    ui->pushButtonNavigationPgroup->setIcon(QIcon(":/images/login.svg"));
    ui->pushButtonNavigationPgroup->setIconSize(QSize(50,50));

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

    ui->pushButtonNavigationPgroup->installEventFilter(pgroupWatcher);
    ui->pushButtonGoToArchive->installEventFilter(archiveWatcher);
    ui->pushButtonGoToRetrieve->installEventFilter(retrieveWatcher);



    ui->tableWidgetRetriever->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetRetriever->insertColumn(0);
    ui->tableWidgetRetriever->insertColumn(1);
    ui->tableWidgetRetriever->insertColumn(2);
    ui->tableWidgetRetriever->setColumnWidth(0, 200);
    ui->tableWidgetRetriever->setColumnWidth(1, 100);
    ui->tableWidgetRetriever->setColumnWidth(2, 100);
    ui->tableWidgetRetriever->setHorizontalHeaderLabels(QStringList({"Name", "Retrievable", "Retrieved"}));


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
            ui->pushButtonNavigationPgroup->setIcon(QIcon(":/images/login-not-active.svg"));
            ui->pushButtonNavigationPgroup->setIconSize(QSize(50,50));
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
            ui->pushButtonNavigationPgroup->setIcon(QIcon(":/images/login-active.svg"));
            ui->pushButtonNavigationPgroup->setIconSize(QSize(50,50));
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
    if (checkPIInfo()){
        if (secID!=2){
            updateNavigationIcons(2);
        }
        toggleNavigationBar(2);
        ui->stackedWidget->setCurrentIndex(numPagesLogin+numPagesArchive);
        ui->nextSlideButton->show();
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
    }
}

void Dialog::goToMetadata(){
    if (checkPIInfo() && filelistingCheckAndUpdate()){
        goToPage(numPagesLogin+1);
    }
}

void Dialog::goToSubmit(){
    if (checkPIInfo() && filelistingCheckAndUpdate() && checkMetadata()){
        goToPage(numPagesLogin+2);
        ui->pushButtonRestart->setText("Submit job");
        ui->waitingMessage->hide();
        ui->nextSlideButton->setText("Done");
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
        if (ui->lineEditDatasetName->text().isEmpty()){
            QMessageBox::information(this, tr("Invalid entry"), tr("Dataset name cannot be empty.") );
            ui->metadataGuided->setCurrentIndex(0);
            ui->labelDatasetName->setFocus();
            return false;
        }
    }
    metadataContainer.datasetName = ui->lineEditDatasetName->text();
    return true;
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
    ui->lineEditProposalID->setText(QString::fromStdString(metadataContainer.proposalID));
    ui->lineEditInvestigator->setText(metadataContainer.investigator);
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Proposals/" + QString::fromStdString(metadataContainer.proposalID).replace("/", "%2F").toStdString() + "?access_token=" + token + "'");
    metadataContainer.creationLocation = QString::fromStdString(utils::extractFromJson(tmp, "instrument"));

    ui->comboBoxCreationLocation->setCurrentIndex(ui->comboBoxCreationLocation->findText(metadataContainer.creationLocation));

    filelistingContainer.clear();
    for (int ii=0; ii<ui->filelistingListWidget->count(); ii++ ){
        filelistingContainer.push_back(ui->filelistingListWidget->item(ii)->text());
    }

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
    ui->pushButtonNavigationPgroup->setEnabled(enabled);
    ui->pushButtonNavigationFiles->setEnabled(enabled);
    ui->pushButtonNavigationMetadata->setEnabled(enabled);
    ui->pushButtonNavigationSubmit->setEnabled(enabled);
}

void Dialog::prepareIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    QString dirpath = settingsContainer.value("logPath", defaultSettings.logPath).toString() + metadataContainer.datasetName + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            sendWarning("Could not write to output directory.");
        }
    }

    QFile metadataFile(QDir(dirpath).filePath(settingsContainer.value("filenameMetadata", defaultSettings.filenameMetadata).toString()));
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        sendWarning("Could not open metadata file.");
        return;
    }

    // prepare metadata file
    QJsonObject metadata;
    bool isRawData = (bool) ui->comboBoxDatatype->currentIndex();
    auto sourceFolder = extractSourceFolder(filelistingContainer);
    metadata["sourceFolder"] = sourceFolder;
    metadata["owner"] = QString::fromStdString(metadataContainer.owner);
    metadata["ownerGroup"] = QString::fromStdString(metadataContainer.ownerGroup);
    metadata["ownerEmail"] = QString::fromStdString(metadataContainer.ownerEmail);
    metadata["description"] = metadataContainer.description;

    if (isRawData){
        qDebug() << "Preparing metadata file for raw data";
        metadata["principalInvestigator"] = QString::fromStdString(metadataContainer.PIemail); // this should be changed in the future; there is no need to save it as std::string
        metadata["creationLocation"] = metadataContainer.creationLocation;
        metadata["datasetName"] = metadataContainer.datasetName;
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
        metadata["inputDatasets"] = QJsonArray::fromStringList(inputDatasetsList);
        metadata["usedSoftware"] = metadataContainer.usedSoftware;
    }


    QJsonDocument saveDoc(metadata);
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

    QFile filelistingFile(QDir(dirpath).filePath(settingsContainer.value("filenameFilelisting", defaultSettings.filenameFilelisting).toString()));
    if (!filelistingFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        sendWarning("Could not open filelisting file.");
        return;
    }

    QTextStream out(&filelistingFile);
    for (auto v:filelistingContainer){
        out << v.remove(sourceFolder) << "\n";
    }
    filelistingFile.close();
}

void Dialog::startIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    ui->waitingMessage->show();
    ui->waitingCircle->setMovie(movie);
    ui->waitingCircle->show();
    movie->start();

    // disable buttons
    setButtonsEnabled(false);

    // compile ingestion command
    QStringList arguments;

    if (ui->checkBoxIngest->isChecked()>0){
        arguments << "--ingest";
    }
    if (ui->checkBoxArchive->isChecked()>0){
        arguments << "--autoarchive";
    }

    arguments << "--noninteractive" << "--allowexistingsource" << "--token" << QString::fromStdString(token);

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
    ui->plainTextEdit->appendPlainText("Error: " + (QString) error);
    ui->plainTextEdit->appendPlainText(ingestion->errorString());
    qDebug() << "Error: " << error;
    ingestionFinished(1);
}

void Dialog::readIngestionOutput(){
    while(ingestion->canReadLine()){
        ui->plainTextEdit->appendPlainText(ingestion->readLine());
    }

}

void Dialog::ingestionFinished(int exitCode){
    movie->stop();
    ui->waitingCircle->hide();
    ui->nextSlideButton->setEnabled(true);
    ingested = true;
    ui->pushButtonRestart->setText("Restart");

    if (exitCode==0){
        // ingestion finished, reenable buttons
        ui->pleaseWait->setText("Done.");
    } else {
        ui->pleaseWait->setText("Ingestion failed.");
    }

}

void Dialog::resetContent(){
    setButtonsEnabled(true);
    filelistingContainer.clear();
    clearAllDatasets();
    clearAllFilelisting();
    ui->metadataGuided->setCurrentIndex(0);
    ui->nextSlideButton->setText("Next");
    ingested = false;
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
    if ((PIHasChanged) || (!PIIsValid)){
        qDebug() << "checking PI";
        PIHasChanged = false;
        PIIsValid = checkPgroup(); // this should also check the PI info, not only the pgroup
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

            // set defaults for owner/ownergroup etc
            metadataContainer.ownerGroup = metadataContainer.pgroup;
            metadataContainer.owner = metadataContainer.PIFirstname + " " + metadataContainer.PILastname;
            metadataContainer.ownerEmail = metadataContainer.PIemail;
            auto tmpUser = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Users/userInfos?access_token=" + token + "'");

            settingsContainer.value("investigatorIsPI", defaultSettings.investigatorIsPI).toBool() ? metadataContainer.investigator=QString::fromStdString(metadataContainer.PIemail) : metadataContainer.investigator=QString::fromStdString(utils::extractFromJson(tmpUser, "currentUserEmail"));
            ui->lineEditPIMail->setText(QString::fromStdString(metadataContainer.PIemail));
            ui->lineEditPIFirstname->setText(QString::fromStdString(metadataContainer.PIFirstname));
            ui->lineEditPILastname->setText(QString::fromStdString(metadataContainer.PILastname));
            ui->lineEditProposalID_2->setText(QString::fromStdString(metadataContainer.proposalID));


        }

        ui->lineEditPIMail->setEnabled(true);
        ui->lineEditPIFirstname->setEnabled(true);
        ui->lineEditPILastname->setEnabled(true);
        ui->lineEditProposalID_2->setEnabled(true);

        ui->pushButtonArchiveData->show();
        ui->pushButtonRetrieveData->show();

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

void Dialog::json2TreeModel(QTreeWidgetItem *item, QJsonObject &inObj){
    for (auto v:inObj){
        qDebug() << v.isArray();
        qDebug() << v;
        if (v.isArray()){
            for (int ii=0; ii<v.toArray().size(); ii++){
                qDebug() << v.toArray()[ii].isObject();
                auto entries = v.toArray()[ii];
                if (entries.isObject()){
                    if (entries.toObject().size()==1){
                        addEntryMetadataTreeView(entries.toObject().keys()[0],entries.toObject().value(entries.toObject().keys()[0]).toString(),item, false, 4);
                    } else {
//                        json2TreeModel()
                    }
                } else {
                    qDebug() << entries;
//                    json2TreeModel(item, entries.toObject());
                }

            }

        }
        v.toArray().size();
    }
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
    json2TreeModel(nullptr, jsonObj);
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
    if (ui->treeWidget->selectedItems().size()>0){
        if (ui->treeWidget->currentItem()->flags().testFlag(Qt::ItemIsDropEnabled)){
            ui->treeWidget->currentItem()->setData(1,0,"");
            addEntryMetadataTreeView("name", "value", ui->treeWidget->currentItem(), true, 4);

        } else {
            sendWarning("Cannot append child entry to protected field.");
        }
    } else {
        addEntryMetadataTreeView("name", "value", "scientificMetadata", true, 4);
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
    QSettings settingsContainer("PSI", "SciCat");
    auto currentDatasets = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Datasets?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.ownerGroup + "%22%7D%2C%22fields%22%3A%7B%22datasetlifecycle%22%3Atrue%2C%20%22pid%22%3Atrue%2C%20%22datasetName%22%3Atrue%7D%7D&access_token=" + token + "'");
    auto successfulJobs = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "Jobs?filter=%7B%22where%22%3A%20%7B%22and%22%3A%5B%7B%22type%22%3A%22retrieve%22%7D%2C%20%7B%22emailJobInitiator%22%3A%20%22" + metadataContainer.userMail.replace("@", "%40").toStdString() + "%22%7D%5D%7D%2C%22fields%22%3A%7B%22datasetList%22%3Atrue%2C%20%22jobStatusMessage%22%3Atrue%7D%7D&access_token=" + token + "'");
//    qDebug() << "curl -s -X GET --header 'Accept: application/json' '" + defaultSettings.serverList[settingsContainer.value("serverID", defaultSettings.serverID).toInt()].toStdString() + "/Jobs?filter=%7B%22where%22%3A%20%7B%22and%22%3A%5B%7B%22type%22%3A%22retrieve%22%7D%2C%20%7B%22emailJobInitiator%22%3A%20%22" + metadataContainer.userMail.replace("@", "%40").toStdString() + "%22%7D%5D%7D%2C%22fields%22%3A%7B%22datasetList%22%3Atrue%2C%20%22jobStatusMessage%22%3Atrue%7D%7D&access_token=" + token + "'";
    // get all 'finishedSuccessful' jobs
    // sadly, there seems to be no option to filter the jobs by pgroup, so we have to query all and select the ones from the current account
    QJsonDocument successfulJobsJson = QJsonDocument::fromJson(QString::fromStdString(successfulJobs).toUtf8());
    std::vector<QString>finishedSuccessfulPIDs;
    for (auto v:successfulJobsJson.array()){
        if (v.toObject().find("jobStatusMessage").value().toString().compare("finishedSuccessful")==0){
            auto datasetListArray = v.toObject().take("datasetList").toArray();
            for (int ii=0; ii<datasetListArray.size(); ii++) {
                finishedSuccessfulPIDs.push_back(datasetListArray.at(ii).toObject().find("pid").value().toString());
            }
        }
    }

    QJsonDocument currentDatasetsJson = QJsonDocument::fromJson(QString::fromStdString(currentDatasets).toUtf8());
    std::vector<retrieveContainer_t> retrieveContainer;
    for (auto v : currentDatasetsJson.array()){
        retrieveContainer_t tmp;
        tmp.pid = v.toObject().find("pid").value().toString();
        tmp.name = v.toObject().find("datasetName").value().toString();
        bool retrievableBool = v.toObject().find("datasetlifecycle").value().toObject().find("retrievable").value().toBool();
        retrievableBool ? tmp.retrievable="true" : tmp.retrievable="false";
        if (std::find(finishedSuccessfulPIDs.begin(), finishedSuccessfulPIDs.end(), tmp.pid) != finishedSuccessfulPIDs.end()) {
          tmp.retrieved = "true";
        }
//        qDebug() << tmp;
        retrieveContainer.push_back(tmp);
    }


    ui->tableWidgetRetriever->setRowCount(0);
    ui->tableWidgetRetriever->clear();
    ui->tableWidgetRetriever->setSortingEnabled(false);
    for (auto v : retrieveContainer) {
        ui->tableWidgetRetriever->insertRow(0);
        QTableWidgetItem *tmpItemDataset = new QTableWidgetItem(v.name);
        ui->tableWidgetRetriever->setItem(0,0,tmpItemDataset);
        QTableWidgetItem *tmpItemRetrievable = new QTableWidgetItem(v.retrievable);
        ui->tableWidgetRetriever->setItem(0,1,tmpItemRetrievable);
        QTableWidgetItem *tmpItemStatus = new QTableWidgetItem(v.retrieved);
        ui->tableWidgetRetriever->setItem(0,2,tmpItemStatus);
    }
    ui->tableWidgetRetriever->setSortingEnabled(true);

}

void Dialog::openSettings(){
    Settings sett(this);
    sett.connectToUi(this);
    sett.exec();
}


void Dialog::quit(){
    close();
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
    QSettings settingsContainer("PSI", "SciCat");

    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();
    std::string token;
    auto tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverListAuth[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "'");
    qDebug() << tmp;
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        // try BL accounts
        tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "Users/login'");
        if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
            QMessageBox::information(this, tr("Authentication error"), tr("Please check your username and password") );
            uiLogin->lineEditPassword->clear();
            return;
        } else {
            baseUi->metadataContainer.isBeamlineAccount = true;
        }
    }


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
        auto tmpUserInfo = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "/Users/userInfos?access_token=" + token + "'");
        baseUi->metadataContainer.userMail = QString::fromStdString(utils::extractFromJson(tmpUserInfo, "currentUserEmail"));
        baseUi->metadataContainer.username = QString::fromStdString(utils::extractFromJson(tmpUserInfo, "currentUser"));
        baseUi->metadataContainer.userInfo = tmpUserInfo;
        baseUi->ui->labelUsername->setText("Username: " + baseUi->metadataContainer.username);
        baseUi->ui->labelServer->setText("Server: " + baseUi->defaultSettings.serverListNames[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()]);
        //            qDebug() << "curl -s -X GET --header 'Accept: application/json' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "Users/userInfos?access_token=" + token + "'";
        //            qDebug() << baseUi->metadataContainer.userMail;
//                    qDebug() << tmpUserInfo;
        close();
        baseUi->token = token;
        baseUi->getAvailPgroups();
        baseUi->show();
        qDebug() << token;
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


Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(acceptChanges()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui->checkBoxEnableAdvanced, SIGNAL(stateChanged(int)), this, SLOT(toggleAdvancedSettings(int)));
    connect(ui->comboBoxServer, SIGNAL(currentIndexChanged(int)), this, SLOT(updateServerURL(int)));
    connect(ui->pushButtonBrowseLogPath, SIGNAL(clicked()), this, SLOT(logFileBrowser()));
    connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
    toggleAdvancedSettings(0);
    ui->tabWidget->setCurrentIndex(0);

}

void Settings::reset(){
    ui->lineEditMetadataFile->setText(baseUi->defaultSettings.filenameMetadata);
    ui->lineEditFilelistingFile->setText(baseUi->defaultSettings.filenameFilelisting);
    ui->lineEditLogPath->setText(baseUi->defaultSettings.logPath);
    ui->checkBoxDeleteLog->setChecked(baseUi->defaultSettings.deleteLog);
    ui->checkBoxPrintCalls->setChecked(baseUi->defaultSettings.printIngestion);
    ui->checkBoxSystemCalls->setChecked(baseUi->defaultSettings.disableSystemCalls);
    ui->comboBoxServer->setCurrentIndex(baseUi->defaultSettings.serverID);
    updateServerURL(baseUi->defaultSettings.serverID);

    updateSettings();
}

void Settings::logFileBrowser(){
    auto fileName = QFileDialog::getExistingDirectory(this,tr("Select log file path"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (fileName.length()>0){
        ui->lineEditLogPath->setText(fileName);
    }
}

void Settings::updateServerURL(int val){
    ui->lineEditServer->setText(baseUi->defaultSettings.serverList[val]);
}

void Settings::acceptChanges(){
    QSettings settingsContainer("PSI", "SciCat");

    // update settings
    if (settingsContainer.value("serverID").toInt()!=ui->comboBoxServer->currentIndex()){
        QMessageBox msgBox;
        msgBox.setText("You must restart the application to apply these changes.");
        msgBox.exec();
    }
    updateSettings();
    close();
}

void Settings::updateSettings(){
    // update settingsContainer
    QSettings settingsContainer("PSI", "SciCat");
    settingsContainer.setValue("filenameFilelisting", ui->lineEditFilelistingFile->text());
    settingsContainer.setValue("filenameMetadata", ui->lineEditMetadataFile->text());
    settingsContainer.setValue("logPath", ui->lineEditLogPath->text());
    settingsContainer.setValue("deleteLog", (bool) ui->checkBoxDeleteLog->checkState());
    settingsContainer.setValue("printIngestion", (bool) ui->checkBoxPrintCalls->checkState());
    settingsContainer.setValue("disableSystemCalls", (bool) ui->checkBoxSystemCalls->checkState());
    settingsContainer.setValue("serverID", ui->comboBoxServer->currentIndex());
}

void Settings::toggleAdvancedSettings(int val){
    bool enableSettings;
    val > 0 ? enableSettings = true : enableSettings = false;

    ui->groupBoxServer->setEnabled(enableSettings);
    ui->groupBoxDebugging->setEnabled(enableSettings);

}

void Settings::connectToUi(Dialog *uiDialog){
    baseUi = uiDialog;
    ui->lineEditServer->setReadOnly(true);

    //update UI
    QSettings settingsContainer("PSI", "SciCat");
    ui->lineEditMetadataFile->setText(settingsContainer.value("filenameMetadata").toString());
    ui->lineEditFilelistingFile->setText(settingsContainer.value("filenameFilelisting").toString());
    ui->lineEditLogPath->setText(settingsContainer.value("logPath").toString());
    ui->checkBoxDeleteLog->setChecked(settingsContainer.value("deleteLog").toBool());
    ui->checkBoxPrintCalls->setChecked(settingsContainer.value("printIngestion").toBool());
    ui->checkBoxSystemCalls->setChecked(settingsContainer.value("disableSystemCalls").toBool());
    ui->comboBoxServer->setCurrentIndex(settingsContainer.value("serverID").toInt());
    updateServerURL(settingsContainer.value("serverID").toInt());


}

Settings::~Settings()
{
    delete ui;
}

void Ingestor::prepareIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    QString dirpath = settingsContainer.value("logPath", baseUi->defaultSettings.logPath).toString() + baseUi->metadataContainer.datasetName + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            baseUi->sendWarning("Could not write to output directory.");
        }
    }

    QFile metadataFile(QDir(dirpath).filePath(settingsContainer.value("filenameMetadata", baseUi->defaultSettings.filenameMetadata).toString()));
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
        metadata["type"] = "derived";
        metadata["ownerGroup"] = QString::fromStdString(baseUi->metadataContainer.ownerGroup);
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

    QFile filelistingFile(QDir(dirpath).filePath(settingsContainer.value("filenameFilelisting", baseUi->defaultSettings.filenameFilelisting).toString()));
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

Ingestor::Ingestor(const QString &commandIn){
    command = commandIn;
}

void Ingestor::connectToUi(Dialog *uiDialog){
    baseUi = uiDialog;
}

void Ingestor::run(){
    utils::system_call(command.toStdString());
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


