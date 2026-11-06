#include "dialog.h"

#include <QDateTime>
#include <QPainter>
#include <tuple>
#include <QMetaEnum>
#include <memory>
#include "messagehandlergui.h"



extern QVersionNumber currentVersionNumber;

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    ui->setupUi(this);
    this->setWindowFlag(Qt::WindowContextHelpButtonHint,false);

    // TODO hidden objects where need to be fixed/implemented
    ui->pushButtonDetach->hide();
    ui->pushButtonKill->hide();
    // end

    ui->labelLogo->setPixmap(QIcon(":/images/SciCat_logo_icon.svg").pixmap(QSize(50, 50), devicePixelRatioF()));


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

    pgroupWatcher = new ButtonHoverWatcher(this);
    archiveWatcher = new ButtonHoverWatcher(this);
    retrieveWatcher = new ButtonHoverWatcher(this);
    settingsWatcher = new ButtonHoverWatcher(this);

    loginIcons = navigationIcons(":/images/nav-pgroup.svg");
    archiveIcons = navigationIcons(":/images/nav-archive.svg");
    retrieveIcons = navigationIcons(":/images/nav-retrieve.svg");
    settingsIcons = navigationIcons(":/images/nav-settings.svg");
    const std::vector<std::tuple<QPushButton*, ButtonHoverWatcher*, const std::vector<QIcon>*, NavIcon>> navButtons{
        {ui->pushButtonGoToPgroup, pgroupWatcher, &loginIcons, NavIconActive},
        {ui->pushButtonGoToArchive, archiveWatcher, &archiveIcons, NavIconInactive},
        {ui->pushButtonGoToRetrieve, retrieveWatcher, &retrieveIcons, NavIconInactive},
        {ui->pushButtonSettings, settingsWatcher, &settingsIcons, NavIconInactive}};
    for (const auto& [button, watcher, icons, initial] : navButtons){
        watcher->setIcons(*icons);
        button->setIcon((*icons)[initial]);
        button->setIconSize(NavIconSize);
        button->installEventFilter(watcher);
    }
    pgroupWatcher->setButtonIsActive(true);

    QSettings settingsContainer("PSI", "SciCat");

    // flush the settings container if the major version number has changed
    QVersionNumber oldVersion = QVersionNumber::fromString(settingsContainer.value("version", QVersionNumber(-1,0).toString()).toString());
    if (oldVersion.isNull() || (oldVersion.majorVersion()!=currentVersionNumber.majorVersion())){
        qDebug() << "Current version number has changed. Flushing settings container.";
        settingsContainer.clear();
    }

    updateSettingsContainer(settingsContainer, "version", currentVersionNumber.toString());
    updateSettingsContainer(settingsContainer, "investigatorIsPI", defaultSettings.investigatorIsPI);
    updateSettingsContainer(settingsContainer, "scicatCliBinary", defaultSettings.scicatCliBinary);
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

    // initialize SciCat container
    scicat = new SciCat::DataCatalogue(this, settingsContainer.value("serverID", defaultSettings.serverID).toInt());


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
    // the platform's own reload icon, so the button matches the native style
    ui->pushButtonRefreshJobs->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(ui->tableWidgetRetriever, SIGNAL(itemSelectionChanged()), this, SLOT(updateRetrieverFiles()));
    connect(ui->pushButtonToggleFilesMetadata, SIGNAL(clicked()), this, SLOT(toggleFilesMetadata()));
    connect(scicat->ingestion(), SIGNAL(finished(int)), this, SLOT(ingestionFinished(int)));
    connect(scicat->ingestion(), SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ingestionFailed(QProcess::ProcessError)));



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
    addEntryMetadataTreeView("sample", "", "scientificMetadata", false, 4);
    addEntryMetadataTreeView("description", "", "sample", false, 4);
    addEntryMetadataTreeView("name", "", "sample", false, 4);
    addEntryMetadataTreeView("principalInvestigator", scicat->experiment()->getPIemail(), "sample", false, 4);

    ui->waitingCircle->setText("Dummy");
    if (!movie->isValid())
        return;



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

    ui->tableWidgetRetrieverFiles->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetRetrieverFiles->insertColumn(0);
    ui->tableWidgetRetrieverFiles->insertColumn(1);
    ui->tableWidgetRetrieverFiles->insertColumn(2);

    ui->tableWidgetRetrieverFiles->setHorizontalHeaderLabels(QStringList({"File", "Size", "Date"}));
    ui->treeWidgetScientificMetadataRetrieve->setColumnCount(2);
    ui->treeWidgetScientificMetadataRetrieve->invisibleRootItem()->setFlags((Qt::ItemFlags)32);
    ui->treeWidgetScientificMetadataRetrieve->setHeaderLabels(QStringList({"Name", "Value"}));


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


    // hide file list and metadata
    ui->treeWidgetScientificMetadataRetrieve->setVisible(false);
    ui->tableWidgetRetrieverFiles->setVisible(false);
    ui->horizontalSpacer_22->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->pushButtonToggleFilesMetadata->setText("Show metadata");


}

void Dialog::sendWarning(QString warningMsg){
    ui->warningLabel->setText(warningMsg);
    ui->warningLabel->setVisible(true);
    QTimer::singleShot(5000, ui->warningLabel, &QLabel::hide);
}

void Dialog::sendMessage(QString msg){
    ui->warningLabel->setText(msg);
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

        switch (secID==-1 ? 0 : secID) {
        case 0:{
            ui->pushButtonGoToPgroup->setIcon(loginIcons[NavIconInactive]);
            ui->pushButtonGoToPgroup->setIconSize(NavIconSize);
            pgroupWatcher->setButtonIsActive(false);
            break;
        }

        case 1: {
            ui->pushButtonGoToArchive->setIcon(archiveIcons[NavIconInactive]);
            ui->pushButtonGoToArchive->setIconSize(NavIconSize);
            archiveWatcher->setButtonIsActive(false);
            break;
        }

        case 2:{
            ui->pushButtonGoToRetrieve->setIcon(retrieveIcons[NavIconInactive]);
            ui->pushButtonGoToRetrieve->setIconSize(NavIconSize);
            retrieveWatcher->setButtonIsActive(false);
            break;
        }

        }
        switch (targetID) {
        case 0:{
            ui->pushButtonGoToPgroup->setIcon(loginIcons[NavIconActive]);
            ui->pushButtonGoToPgroup->setIconSize(NavIconSize);
            pgroupWatcher->setButtonIsActive(true);
            break;
        }
        case 1: {
            ui->pushButtonGoToArchive->setIcon(archiveIcons[NavIconActive]);
            ui->pushButtonGoToArchive->setIconSize(NavIconSize);
            archiveWatcher->setButtonIsActive(true);
            break;
        }
        case 2: {
            ui->pushButtonGoToRetrieve->setIcon(retrieveIcons[NavIconActive]);
            ui->pushButtonGoToRetrieve->setIconSize(NavIconSize);
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
    scicat->experiment()->setInvestigator(ui->lineEditInvestigator->text());
}

void Dialog::descriptionChanged(){
    scicat->experiment()->setDescription(ui->textEditDescription->toPlainText());
}

void Dialog::usedSoftwareChanged(){
    scicat->experiment()->setUsedSoftware(ui->textEditUsedSoftware->toPlainText());
}

void Dialog::setAvailPgroups(){
    // set autocomplete for pgroup selection
    pgroupCompleter = new QCompleter(scicat->user()->getPgroups(), this);
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
//    int slideIndex = ui->stackedWidget->currentIndex();
//    qDebug() <<  "current index: " << slideIndex;
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
    qDebug() << "checking metadata";
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

    scicat->experiment()->setDatasetName(ui->lineEditDatasetName->text());
    return true;
}

void Dialog::creationLocationChanged(int val){
    (void) val;
    if (ui->stackedWidget->currentIndex()==2){
        if (val==ui->comboBoxCreationLocation->findText("custom")){
            // open creationLocationCreator
            CreationLocationCreator creationLoc(this);
            creationLoc.exec();
        } else {
            scicat->experiment()->setCreationLocation(ui->comboBoxCreationLocation->currentText());
        }
    }
}

bool Dialog::filelistingCheckAndUpdate(){
    QSettings settingsContainer("PSI", "SciCat");

    if (ui->filelistingListWidget->count()==0){
        QMessageBox::information(this, tr("Invalid entry"), tr("Please add at least one file/directory.") );
        return false;
    }
    // prepare metadata entries
    ui->lineEditOwnerEmail->setText(scicat->experiment()->getOwnerEmail());
    ui->lineEditOwner->setText(scicat->experiment()->getOwner());
    ui->lineEditOwnerGroup->setText(scicat->experiment()->getOwnerGroup());

    if (scicat->experiment()->getProposalID().length()>0){
        ui->lineEditProposalID->setText(scicat->experiment()->getProposalID());
        ui->comboBoxCreationLocation->setCurrentIndex(ui->comboBoxCreationLocation->findText(scicat->experiment()->getCreationLocation()));
    } else {
        if (scicat->experiment()->getCreationLocation().length()==0){
            ui->comboBoxCreationLocation->setCurrentIndex(-1);
        }
    }
    ui->lineEditInvestigator->setText(scicat->experiment()->getInvestigator());

    filelistingContainer.clear();
    for (int ii=0; ii<ui->filelistingListWidget->count(); ii++ ){
        filelistingContainer.push_back(ui->filelistingListWidget->item(ii)->text());
    }

    // guess dataset name
    if (settingsContainer.value("guessDatasetname", defaultSettings.guessDatasetname).toBool()){
        if (ui->lineEditDatasetName->text().length()==0){
            if (filelistingContainer.size()==1){
                QFileInfo info(filelistingContainer[0]);
                ui->lineEditDatasetName->setText(info.baseName());

            } else if (filelistingContainer.size()>1){
                auto sourceFolder = extractSourceFolder(filelistingContainer);
                QFileInfo info(sourceFolder);
                ui->lineEditDatasetName->setText(info.baseName());
            }
        }
    }



    // count files
    connect(&watcherFileCounter, SIGNAL(finished()), this, SLOT(fileCounterFinished()));
    futureGetFileNum = QtConcurrent::run(getFileNum, filelistingContainer);
    watcherFileCounter.setFuture(futureGetFileNum);

//    int count = 0;
//    for (auto v:filelistingContainer) {
//        count += countFilesAndDirRecursive(v);
//    }
//    fileCounter_t res = countFilesAndDirRecursive(filelistingContainer[0]);
//    count = res.numFiles;
//    if (count>defaultSettings.maxFileNum){
//        QMessageBox::information(this, tr("Warning"), tr("The selected directories exceed the maximum number of files per dataset. Your dataset will be split into smaller datasets.") );
//    }
//    qDebug() << "Number of files: " << count;


//    for (auto v:res.subDir){
//        qDebug() << v.name << " " << v.numFiles;
//        if (v.numFiles>defaultSettings.maxFileNum){
//            for (auto sub:v.subDir){
//                qDebug() << sub.name << " " << sub.numFiles;
//            }
//        }
//    }


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


namespace {
QString processErrorToString(QProcess::ProcessError error){
    const QMetaEnum metaEnum = QMetaEnum::fromType<QProcess::ProcessError>();
    const char* key = metaEnum.valueToKey(error);
    return key ? QString::fromLatin1(key) : QString::number(static_cast<int>(error));
}
}

void Dialog::ingestionFailed(QProcess::ProcessError error){
    ingestionLog->ui->textEditIngestionLog->append("Error: " + processErrorToString(error));
    ingestionLog->ui->textEditIngestionLog->append(scicat->ingestion()->getIngestJob()->getJob()->errorString());
    qDebug() << "Error: " << error;
    ingestionFinished(1);
    ingestionIsRunning = false;
}


void Dialog::ingestionFinished(int exitCode){
    QSettings settingsContainer("PSI", "SciCat");

    movie->stop();
    ui->waitingCircle->hide();
    ui->nextSlideButton->setEnabled(true);
    ingested = true;
    ui->pushButtonRestart->setEnabled(true);
    ui->pushButtonRestart->setText("Restart");
    ingestionIsRunning = false;

    if (exitCode==0){
        // ingestion finished, reenable buttons
        ui->pleaseWait->setText("Done.");
    } else {
        ui->pleaseWait->setText("Ingestion failed.");
    }

    if (settingsContainer.value("deleteLog", defaultSettings.deleteLog).toBool()){
        qDebug() << "removing ingestion log files";
//        qDebug() << metadataContainer.ingestionLogDir;
        if (!QDir(scicat->ingestion()->getIngestionLogDir()).removeRecursively()){
            MessageHandlerGUI::sendWarning(ui->warningLabel, "Failed to remove ingestion log files.");
        }
    } else if (settingsContainer.value("saveLog", defaultSettings.saveLog).toBool()){
        ingestionLog->saveLog(scicat->ingestion()->getIngestionLogDir() + "/ingestion.log");
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
    ui->lineEditDatasetName->setText("");
}

void Dialog::restart(){
    if (ingested){
        resetContent();
        goToPgroup(); // go back to PI
    } else {
        ui->waitingMessage->show();
        ui->pleaseWait->setText("Please wait.");
        ui->waitingCircle->setMovie(movie);
        ui->waitingCircle->show();
        movie->start();
        startIngestionWhenReady = false;
        if (!futureGetFileNum.isFinished()){
            startIngestionWhenReady = true;
        } else {
            fileCounterFinished(true);
        }

    }
}

void Dialog::fileCounterFinished(bool force){
    if ((startIngestionWhenReady) || force){
        int result = futureGetFileNum.result();
        if (result<defaultSettings.maxFileNum){
            // disable buttons
            setButtonsEnabled(false);
            ui->pushButtonRestart->setEnabled(false);
            if (!ingestionIsRunning){
                ingestionIsRunning = true;
                scicat->ingestion()->prepareIngestion();
                scicat->ingestion()->start(ingestionLog->ui->textEditIngestionLog);
            } else {
                qDebug() << "Ingestion is already running.";
            }
            startIngestionWhenReady = false;
        } else {
            auto maxnum = QString::number(defaultSettings.maxFileNum).toUtf8();
            QMessageBox::information(this, tr("Error"), tr("Maximum number of " + maxnum + " files per dataset exceeded. Please reduce the number of files and try again.") );
            movie->stop();
            ui->waitingCircle->hide();
            ui->nextSlideButton->setEnabled(true);
            ui->pushButtonRestart->setEnabled(true);
            ui->pushButtonRestart->setText("Restart");
            goToFiles();
        }
    }
}

void Dialog::updateRetrieverFiles(){
    qDebugLine("Width: ") << (int)ui->tableWidgetRetrieverFiles->columnWidth(0);
    qDebug() << "Updating retriever files";
    // get current datasetID
    QModelIndexList indexList = ui->tableWidgetRetriever->selectionModel()->selectedRows();
    if (indexList.length()==0){
        return;
    }
    std::string datasetID = ui->tableWidgetRetriever->item(indexList[0].row(),3)->data(0).toString().toStdString();
    qDebug() << "datasetID: " << datasetID;

    QJsonDocument requestDatasetInfoJson;
    QJsonDocument requestSMJson;
    auto searchDatasetID = retrieveDataMap.find(datasetID);
    if (searchDatasetID != retrieveDataMap.end()) {
        requestDatasetInfoJson = searchDatasetID->second;
    } else {
        // files
        auto msgHandler = MessageHandlerGUI::sendMessage(ui->warningLabel, "Retrieving dataset info..."); // use scope-based MessageHandler
        QString encodedDatasetID = QUrl::toPercentEncoding(QString::fromStdString(datasetID));
        QNetworkRequest requestDatasetInfo = scicat->apiRequest("datasets/" + encodedDatasetID + "/origdatablocks");
        auto replyInfo = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, requestDatasetInfo);
        requestDatasetInfoJson = QJsonDocument::fromJson(replyInfo.toUtf8());
        retrieveDataMap[datasetID] = requestDatasetInfoJson;

        // scientific metadata
        QJsonObject filterSM{{"fields", QJsonArray{"scientificMetadata"}}};
        QNetworkRequest requestSM = scicat->apiRequest("datasets/" + encodedDatasetID, filterSM);
        auto replySM = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, requestSM);
        requestSMJson = QJsonDocument::fromJson(replySM.toUtf8());
        auto jsonObj = requestSMJson.object();
        ui->treeWidgetScientificMetadataRetrieve->clear();
        json2TreeModel(jsonObj, nullptr, ui->treeWidgetScientificMetadataRetrieve);
        ui->treeWidgetScientificMetadataRetrieve->expandAll();
    }

    ui->tableWidgetRetrieverFiles->setRowCount(0);
    ui->tableWidgetRetrieverFiles->clearContents();
    ui->tableWidgetRetrieverFiles->setSortingEnabled(false);
    for (auto v : requestDatasetInfoJson.array()){
//        qDebug() << v;
        for (auto f: v.toObject()["dataFileList"].toArray()){
            ui->tableWidgetRetrieverFiles->insertRow(0);
            QTableWidgetItem *tmpItemPath = new QTableWidgetItem(f.toObject()["path"].toString());
            tmpItemPath->setFlags((Qt::ItemFlags)33);
            ui->tableWidgetRetrieverFiles->setItem(0,0,tmpItemPath); // setItem takes ownership -- no delete necessary
            QTableWidgetItem *tmpItemSize = new QTableWidgetItem(QString::number(f.toObject()["size"].toInt()));
            tmpItemSize->setFlags((Qt::ItemFlags)33);
            ui->tableWidgetRetrieverFiles->setItem(0,1,tmpItemSize);
            QTableWidgetItem *tmpItemTime = new QTableWidgetItem(f.toObject()["time"].toString());
            tmpItemTime->setFlags((Qt::ItemFlags)33);
            ui->tableWidgetRetrieverFiles->setItem(0,2,tmpItemTime);
        }
    }
//    ui->tableWidgetRetrieverFiles->setColumnWidth()
    ui->tableWidgetRetrieverFiles->setSortingEnabled(true);
    ui->tableWidgetRetrieverFiles->sortItems(0, Qt::SortOrder::AscendingOrder);

    if (resizeRetrieverFiles){
        ui->tableWidgetRetrieverFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        auto minSizeFiles = ui->tableWidgetRetrieverFiles->horizontalHeader()->sectionSize(0);
        auto sizeSize = ui->tableWidgetRetrieverFiles->horizontalHeader()->sectionSize(1);
        auto sizeDate = ui->tableWidgetRetrieverFiles->horizontalHeader()->sectionSize(2);
        int sizeFiles;
        minSizeFiles > ui->tableWidgetRetrieverFiles->width()/2 ? sizeFiles = ui->tableWidgetRetrieverFiles->width()/2 : sizeFiles = minSizeFiles;
        ui->tableWidgetRetrieverFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

        ui->tableWidgetRetrieverFiles->setColumnWidth(0, sizeFiles);
        ui->tableWidgetRetrieverFiles->setColumnWidth(1, sizeSize);
        ui->tableWidgetRetrieverFiles->setColumnWidth(2, sizeDate);
        resizeRetrieverFiles = false;

        ui->treeWidgetScientificMetadataRetrieve->setColumnWidth(0, (int)ui->treeWidgetScientificMetadataRetrieve->width()/2);
        ui->treeWidgetScientificMetadataRetrieve->setColumnWidth(1, (int)ui->treeWidgetScientificMetadataRetrieve->width()/2);
    }




}


void Dialog::toggleFilesMetadata(){
    if (ui->tableWidgetRetrieverFiles->isVisible()){
        ui->treeWidgetScientificMetadataRetrieve->setVisible(false);
        ui->tableWidgetRetrieverFiles->setVisible(false);
        ui->horizontalSpacer_22->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->pushButtonToggleFilesMetadata->setText("Show metadata");
    } else {
        ui->treeWidgetScientificMetadataRetrieve->setVisible(true);
        ui->tableWidgetRetrieverFiles->setVisible(true);
        ui->horizontalSpacer_22->changeSize(20,20,QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->pushButtonToggleFilesMetadata->setText("Hide metadata");
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
    } else if (((ui->lineEditPgroup->text().size() != 6) && (ui->lineEditPgroup->text().size() != 7)) || ((ui->lineEditPgroup->text().toStdString().find("p") != 0) && (ui->lineEditPgroup->text().toStdString().find("a") != 0))){
        QMessageBox::information(this, tr("Error"), tr("Please enter a valid pgroup/agroup") );
        pgroupIsValid = false;
        return pgroupIsValid;
    }

    // check if user has access to the given pgroup
    if (!scicat->user()->getPgroups().contains(ui->lineEditPgroup->text())){
        auto msg = "Cannot access pgroup " + ui->lineEditPgroup->text().toStdString() + ". Permission denied.";
        QMessageBox::information(this, tr("Error"), tr(msg.c_str()));
        pgroupIsValid = false;
    }

    return pgroupIsValid;
}

bool Dialog::checkPIInfo(){
    QSettings settingsContainer("PSI", "SciCat");
    if (!checkPgroup()){
        return false;
    }
    if ((PIHasChanged)){
        qDebug() << "checking PI";
        PIHasChanged = false;

        scicat->experiment()->setPIemail(ui->lineEditPIMail->text());
        scicat->experiment()->setPIFirstname(ui->lineEditPIFirstname->text());
        scicat->experiment()->setPILastname(ui->lineEditPILastname->text());
        scicat->experiment()->setProposalID(ui->lineEditProposalID_2->text());


        if (scicat->experiment()->getPIFirstname().length()==0){
            QMessageBox::information(this, tr("Error"), tr("PI name cannot be empty") );
            return false;
        }

        if (scicat->experiment()->getPILastname().length()==0){
            QMessageBox::information(this, tr("Error"), tr("PI name cannot be empty") );
            return false;
        }

        if (scicat->experiment()->getPIemail().length()==0){
            QMessageBox::information(this, tr("Error"), tr("Please enter a valid email address") );
            return false;
        }

        // set default for investigator
        scicat->experiment()->setInvestigatorToPI(settingsContainer.value("investigatorIsPI", defaultSettings.investigatorIsPI).toBool());

        // save PI info if needed
        if ((scicat->experiment()->getPgroupRequestState()!=SciCat::DataCatalogueCore::PgroupRequestState::ValidSciCatEntry) && (settingsContainer.value("saveProposalInfo", defaultSettings.saveProposalInfo).toBool())){
            if (scicat->experiment()->savePIInfo()>0){
                MessageHandlerGUI::sendWarning(ui->warningLabel, "Failed to write proposal information.");
            }
        }


    }

    return true;

}

void Dialog::getPI(){
    QSettings settingsContainer("PSI", "SciCat");
    if (Dialog::checkPgroup()){
        scicat->experiment()->setPgroup(ui->lineEditPgroup->text());

        auto state = scicat->experiment()->getPgroupInfo();

        switch (state) {
        case SciCat::DataCatalogueCore::PgroupRequestState::ValidSciCatEntry:

            ui->lineEditPIMail->setText(scicat->experiment()->getPIemail());
            ui->lineEditPIFirstname->setText(scicat->experiment()->getPIFirstname());
            ui->lineEditPILastname->setText(scicat->experiment()->getPILastname());
            ui->lineEditProposalID_2->setText(scicat->experiment()->getProposalID());
            ui->lineEditTitle->setText(scicat->experiment()->getProposalTitle());
            ui->textEditAbstract->setText(scicat->experiment()->getProposalAbstract());

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
            break;
        case SciCat::DataCatalogueCore::PgroupRequestState::ValidLogFile:

            ui->labelProposal->hide();
            ui->groupBoxProposal->show();
            ui->labelManualEntry->hide();
            ui->lineEditProposalID_2->hide();
            ui->lineEditTitle->hide();
            ui->textEditAbstract->hide();
            ui->labelTitle->hide();
            ui->labelAbstract->hide();

            ui->lineEditPIMail->setText(scicat->experiment()->getPIemail());
            ui->lineEditPIFirstname->setText(scicat->experiment()->getPIFirstname());
            ui->lineEditPILastname->setText(scicat->experiment()->getPILastname());
            ui->lineEditProposalID_2->setText(scicat->experiment()->getProposalID());

            ui->lineEditPIMail->setEnabled(true);
            ui->lineEditPIFirstname->setEnabled(true);
            ui->lineEditPILastname->setEnabled(true);
            break;
        case SciCat::DataCatalogueCore::PgroupRequestState::Invalid:
            // manual edit
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
            scicat->experiment()->setProposalID("");
            ui->lineEditProposalID_2->setText(scicat->experiment()->getProposalID());

            ui->lineEditPIMail->setEnabled(true);
            ui->lineEditPIFirstname->setEnabled(true);
            ui->lineEditPILastname->setEnabled(true);
        }

        ui->pushButtonArchiveData->show();
        ui->pushButtonRetrieveData->show();


    }
}

void Dialog::fileDialogFilelisting(){
    FileDialogFilelisting  dialog(this);
    dialog.exec();
    QStringList selection = dialog.selectedFiles();
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
    QJsonObject filter{{"where", QJsonObject{{"ownerGroup", scicat->experiment()->getOwnerGroup()}}},
                       {"fields", QJsonArray{"datasetName", "pid"}}};
    QNetworkRequest request = scicat->apiRequest("datasets", filter);
    QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, request);
    QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());

    datasetContainer.clear();
    for (auto v:doc.array()){
        QStringList tmpQStringList;
        tmpQStringList.push_back(v.toObject().value("datasetName").toString());
        tmpQStringList.push_back(v.toObject().value("pid").toString());
        qDebugLine("DatasetBrowser") << tmpQStringList;
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
    auto fileName = QFileDialog::getSaveFileName(this,tr("Export scientific metadata"), QDir(QDir::homePath()).filePath(scicat->experiment()->getPgroup() + "_scientificMetadata.json"), tr("json files (*.json)"));
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
        MessageHandlerGUI::sendWarning(ui->warningLabel, "Could not export metadata file.");
        return;
    }
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

}


void Dialog::importMetadata(){

    auto fileName = QFileDialog::getOpenFileName(this,tr("Import scientific metadata"), QDir::homePath(), tr("json files (*.json)"));
    QFile importFile(fileName);
    if (!importFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        MessageHandlerGUI::sendWarning(ui->warningLabel, "Could not open metadata file.");
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
                MessageHandlerGUI::sendWarning(ui->warningLabel, "Cannot append child entry to protected field.");
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
        MessageHandlerGUI::sendWarning(ui->warningLabel, "Cannot remove protected field.");
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
    // request a list of all datasets from the current pgroup
    QJsonObject filterDatasets{{"where", QJsonObject{{"ownerGroup", scicat->experiment()->getOwnerGroup()}}},
                               {"fields", QJsonArray{"datasetlifecycle", "pid", "datasetName"}}};
    QNetworkRequest requestCurrentDatasets = scicat->apiRequest("datasets", filterDatasets);
    auto currentDatasetsJob = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestCurrentDatasets);

    // the v3 jobs endpoint maps the v3 field names but does not accept a fields filter
    QJsonObject filterJobs{{"where", QJsonObject{{"type", "retrieve"}, {"emailJobInitiator", scicat->user()->getUserMail()}}}};
    QNetworkRequest requestSuccessfulJobs = scicat->apiRequest("jobs", filterJobs);
    auto replyJobs = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestSuccessfulJobs);

    // wait for jobs to finish
    QJsonDocument currentDatasetsJson = QJsonDocument::fromJson(currentDatasetsJob->pull().toUtf8());
    QJsonDocument successfulJobsJson = QJsonDocument::fromJson(replyJobs->pull().toUtf8());


    // get all 'finishedSuccessful' jobs
    // sadly, there seems to be no option to filter the jobs by pgroup, so we have to query all and select the ones from the current account
    std::vector<QString>finishedSuccessfulPIDs;
    for (auto v:successfulJobsJson.array()){
        if (v.toObject().find("jobStatusMessage").value().toString().compare("finishedSuccessful")==0){
            auto datasetListArray = v.toObject().take("datasetList").toArray();
            for (int ii=0; ii<datasetListArray.size(); ii++) {
                finishedSuccessfulPIDs.push_back(datasetListArray.at(ii).toObject().find("pid").value().toString());
            }
        }
    }

    retrieveContainer.clear();
    for (auto v : currentDatasetsJson.array()){
        retrieveContainer_t tmp;
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
        tmpItemDataset->setFlags((Qt::ItemFlags)33);
        ui->tableWidgetRetriever->setItem(0,0,tmpItemDataset);
        QTableWidgetItem *tmpItemRetrievable = new QTableWidgetItem(v.retrievable);
        tmpItemRetrievable->setFlags((Qt::ItemFlags)33);
        ui->tableWidgetRetriever->setItem(0,1,tmpItemRetrievable);
        QTableWidgetItem *tmpItemStatus = new QTableWidgetItem(v.retrieved);
        tmpItemStatus->setFlags((Qt::ItemFlags)33);
        ui->tableWidgetRetriever->setItem(0,2,tmpItemStatus);
        QTableWidgetItem *tmpItemPID = new QTableWidgetItem(v.pid);
        tmpItemPID->setFlags((Qt::ItemFlags)33);
        ui->tableWidgetRetriever->setItem(0,3,tmpItemPID);

    }
    ui->tableWidgetRetriever->setSortingEnabled(true);

}

void Dialog::openSettings(){
    Settings sett(this, Settings::Context::AfterLogin, this);
    sett.exec();
}

void Dialog::openIngestionLog(){
    ingestionLog->exec();
}

void Dialog::quit(){
    if (!ingested){
        auto reply = QMessageBox::question(this, "Quit", "The archiving process has not been submitted yet. Do you really want to quit now?");
        if (reply == QMessageBox::Yes) {
          close();
          return;
        } else {
            return;
        }
    }
    close();
}

void Dialog::requestRetrieve(){
    QSettings settingsContainer("PSI", "SciCat");
    QJsonArray datasetList;

    QModelIndexList indexList = ui->tableWidgetRetriever->selectionModel()->selectedRows();
    for (int ii=0; ii<indexList.size(); ++ii) {
        qDebug() << ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0);
        if (ui->tableWidgetRetriever->item(indexList[ii].row(),1)->data(0).toBool()) { // check retrievable
            qDebug() << (!ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool());
            qDebug() << (settingsContainer.value("requestAlreadyRetrievedData").toBool());
            if ((!ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool()) || (settingsContainer.value("requestAlreadyRetrievedData").toBool())){
                QJsonObject tmpDatasetObject;
                tmpDatasetObject["pid"] = ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0).toString();
                tmpDatasetObject["files"] = QJsonArray();
                datasetList.push_back(tmpDatasetObject);
            } else {
                MessageHandlerGUI::sendWarning(ui->warningLabel, "Some datasets have already been retrieved and will be skipped.");
            }
        } else {
            MessageHandlerGUI::sendWarning(ui->warningLabel, "Some datasets are not in a retrievable state.");
        }
    }

    // start retrieving data from archive; at this point, the data will only be copied to the retrieve server
    if (scicat->requestRetrieve(datasetList)==0){
        QMessageBox::information(this, tr("Retrieve requested"), tr("The retrieval of your data has been requested. You will be notified via email once the job finished successfully. Successfully retrieved datasets can then be copied using the SciCat GUI.") );
    }

}

void Dialog::saveRetrievedData(){
#ifdef WINDOWS_SPECIFIC_TOOLS
    QMessageBox::information(this, tr("Not supported on Windows"), tr("Retrieving data on Windows is currently not supported.") );
#else
    if (!hasKerberosTicket()){
        QMessageBox::warning(this, tr("Kerberos ticket required"),
                             tr("Copying data from the retrieve server needs a valid Kerberos ticket.\n\n"
                                "Run the following in a terminal, then click Save again:\n\n"
                                "kinit %1").arg(scicat->user()->getUsername()));
        return;
    }
    QString dirname = QFileDialog::getExistingDirectory(this, "Save path",QDir::homePath(),QFileDialog::ShowDirsOnly);
    if (dirname.length()>0){
        QModelIndexList indexList = ui->tableWidgetRetriever->selectionModel()->selectedRows();
        for (int ii=0; ii<indexList.size(); ++ii) {
            qDebug() << ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0);
            if (ui->tableWidgetRetriever->item(indexList[ii].row(),2)->data(0).toBool()) { // check retrieve status
                startRetriever(ui->tableWidgetRetriever->item(indexList[ii].row(),3)->data(0).toString(), dirname, ui->tableWidgetRetriever->item(indexList[ii].row(),0)->data(0).toString());
            } else {
                MessageHandlerGUI::sendWarning(ui->warningLabel, "Some datasets have not been retrieved yet.");
            }
        }
    }
#endif

}

#ifndef WINDOWS_SPECIFIC_TOOLS
// if klist cannot answer (missing, hanging or crashing) the copy goes ahead and scicat-cli reports a missing ticket itself
bool Dialog::hasKerberosTicket() const{
    QProcess klist;
    klist.start("klist", QStringList{"-s"});
    if (!klist.waitForStarted(3000)){
        qWarning() << "[Kerberos] could not run klist:" << klist.errorString();
        return true;
    }
    if (!klist.waitForFinished(3000)){
        qWarning() << "[Kerberos] klist did not finish in time:" << klist.errorString();
        klist.kill();
        klist.waitForFinished(1000);
        return true;
    }
    if (klist.exitStatus()!=QProcess::NormalExit){
        qWarning() << "[Kerberos] klist crashed:" << klist.errorString();
        return true;
    }
    return klist.exitCode()==0;
}
#endif

void Dialog::startRetriever(const QString &pid, const QString &outdir, const QString &name){
    std::shared_ptr<SciCat::DatasetRetriever> retriever = std::make_shared<SciCat::DatasetRetriever>(this);

    retriever->start(pid, outdir);

    retrieveJobsContainer_t job;
    job.pid = pid;
    job.outdir = outdir;
    job.name = name;
    job.process = retriever;

    retrieverList.push_back(job);

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
        if ((*it).process->getRetrieverJob()->processId()==0){
//            delete it->process;
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
        qDebug() << v.process->getRetrieverJob()->processId();
        QTableWidgetItem *tmpJobName = new QTableWidgetItem(v.name);
        ui->tableWidgetJobs->setItem(0,0,tmpJobName); // takes ownership -- no delete necessary
        QTableWidgetItem *tmpJobPath = new QTableWidgetItem(v.outdir);
        ui->tableWidgetJobs->setItem(0,1,tmpJobPath);
        QTableWidgetItem *tmpJobPID = new QTableWidgetItem(QString::number(v.process->getRetrieverJob()->processId()));
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
    } else if (ingestion!=nullptr){
        if (ingestion->state()==QProcess::Running){
            auto reply = QMessageBox::question(this, "Quit", "The ingestion job is still running. Do you really want to quit now and abort the ingestion?");
            if (reply == QMessageBox::Yes) {
                QDialog::reject();
                return;
            } else {
                return;
            }
        }
    }
    QDialog::reject();
}

Dialog::~Dialog()
{
    qDebug() << "delete Dialog";
    delete retrieveWatcher;
    delete archiveWatcher;
    delete pgroupWatcher;
    delete scicat;
    delete ingestionLog;
    delete pgroupCompleter;

    delete ui;
}

QString extractSourceFolder(std::vector<QString> container){
// a quick and dirty solution... not very efficient though

    #ifdef WINDOWS_SPECIFIC_TOOLS
    QString lPath = "";
    int ii=0;
    #else
    QString lPath = "/";
    int ii=1;
    #endif


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
            qDebug() << item->child(ii)->data(1,0).type();
            retObj.insert(item->child(ii)->data(0,0).toString(), tmpObject);
        } else {
            retObj.insert(item->child(ii)->data(0,0).toString(), item->child(ii)->data(1,0).toString());
            qDebug() << item->child(ii)->data(1,0).type();

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
            qDebug() << "[Bool] " << entryVal.isBool();
            qDebug() << "[String] " << entryVal.isString();
            qDebug() << "[Double] " << entryVal.isDouble();
            treeItem->setText(1,entryVal.toString());

        }
        if (item!=nullptr){
            item->addChild(treeItem);
        }
    }
}

ButtonHoverWatcher::ButtonHoverWatcher(QObject * parent) :
    QObject(parent)
{
//    qDebug() << "constructed ButtonHoverWatcher";
}

bool ButtonHoverWatcher::eventFilter(QObject * watched, QEvent * event)
{
    QPushButton * button = qobject_cast<QPushButton*>(watched);
    if (!button) {
        return false;
    }

    if (!buttonIsActive){
        if (event->type() == QEvent::Enter) {
            // The push button is hovered by mouse
            button->setIcon(iconsVector[NavIconHover]);
            button->setIconSize(NavIconSize);
            return true;
        }

        if (event->type() == QEvent::Leave){
            // The push button is not hovered by mouse
            button->setIcon(iconsVector[NavIconInactive]);
            button->setIconSize(NavIconSize);
            return true;
        }

    }


    return false;
}

void ButtonHoverWatcher::setIcons(const std::vector<QIcon>& icons){
    iconsVector = icons;
}

namespace {
constexpr QRgb NavTileColour = qRgb(0x00, 0x2D, 0x3B);
constexpr QRgb NavGlyphColour = qRgb(0xFF, 0xFF, 0xFF);
constexpr QRgb NavGlyphMutedColour = qRgb(0x66, 0x81, 0x89);
constexpr QRgb NavGlyphDisabledColour = qRgba(0x66, 0x81, 0x89, 0x60);
constexpr QRgb NavActiveBarColour = qRgb(0x27, 0xAA, 0xE1);
constexpr QRectF NavActiveBar{0, 9, 2.5, 32};

QPixmap navigationTile(const QIcon& glyph, const QColor& colour, bool active, qreal dpr){
    QPixmap tile(NavIconSize * dpr);
    tile.setDevicePixelRatio(dpr);
    tile.fill(QColor(NavTileColour));

    QPainter painter(&tile);
    painter.setRenderHint(QPainter::Antialiasing);
    if (active){
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(NavActiveBarColour));
        painter.drawRoundedRect(NavActiveBar, NavActiveBar.width() / 2, NavActiveBar.width() / 2);
    }

    QPixmap glyphPixmap = glyph.pixmap(NavIconSize, dpr);
    if (!glyphPixmap.isNull()){
        {
            QPainter tint(&glyphPixmap);
            tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            tint.fillRect(QRect(QPoint(0, 0), NavIconSize), colour);
        }
        painter.drawPixmap(0, 0, glyphPixmap);
    }
    return tile;
}
}

std::vector<QIcon> navigationIcons(const QString& glyphPath){
    const QIcon glyph(glyphPath);
    if (glyph.pixmap(NavIconSize).isNull()){
        qWarning() << "Could not render the sidebar icon" << glyphPath << "- is the Qt SVG icon engine plugin deployed?";
    }

    std::vector<QIcon> icons(3);
    for (const qreal dpr : {1.0, 2.0}){
        icons[NavIconHover].addPixmap(navigationTile(glyph, QColor(NavGlyphColour), false, dpr));
        icons[NavIconActive].addPixmap(navigationTile(glyph, QColor(NavGlyphColour), true, dpr));
        icons[NavIconInactive].addPixmap(navigationTile(glyph, QColor(NavGlyphMutedColour), false, dpr));
        const QPixmap disabled = navigationTile(glyph, QColor::fromRgba(NavGlyphDisabledColour), false, dpr);
        for (QIcon& icon : icons){
            icon.addPixmap(disabled, QIcon::Disabled);
        }
    }
    return icons;
}

void ButtonHoverWatcher::setButtonIsActive(bool val){
    buttonIsActive = val;
}



fileCounter_t countFilesAndDirRecursive(QString dirName){
    QDir dir(dirName);
    int count = 0;
    QStringList fileNameList = dir.entryList();
    fileCounter_t curDir;
    for (auto v:fileNameList){

        if ((v.compare(".")!=0) && (v.compare("..")!=0)){ // ignore .. and .
            QFileInfo info(dirName + "/" + v);
            if (info.isDir()){
                fileCounter_t tmp = countFilesAndDirRecursive(dirName + "/" + v);
                curDir.subDir.push_back(tmp);
                count += tmp.numFiles;
            } else {
                ++count;
            }
        }
    }
    curDir.name = dirName;

    curDir.numFiles = count;

    return curDir;
}

int getFileNum(std::vector<QString> container){
    int count = 0;
    for (auto v:container){
        count+= countFilesAndDirRecursive(v).numFiles;
    }
    return count;
}
