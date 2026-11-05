#include "settings.h"

extern QVersionNumber currentVersionNumber;

Settings::Settings(Dialog *base, Context context, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings),
    baseUi(base),
    context(context)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::WindowContextHelpButtonHint,false);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(acceptChanges()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui->checkBoxEnableAdvanced, SIGNAL(stateChanged(int)), this, SLOT(toggleAdvancedSettings(int)));
    connect(ui->comboBoxServer, SIGNAL(currentIndexChanged(int)), this, SLOT(updateServerURL(int)));
    connect(ui->pushButtonBrowseLogPath, SIGNAL(clicked()), this, SLOT(logFileBrowser()));
    connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
    connect(ui->lineEditFilelistingFile, SIGNAL(editingFinished()), this, SLOT(checkFilelistingFile()));
    connect(ui->lineEditMetadataFile, SIGNAL(editingFinished()), this, SLOT(checkMetadataFile()));
    toggleAdvancedSettings(0);
    ui->tabWidget->setCurrentIndex(0);

    ui->lineEditServer->setReadOnly(true);

    //update UI
    ui->labelVersionNumber->setText("v"+currentVersionNumber.toString());
    QSettings settingsContainer("PSI", "SciCat");
    ui->lineEditMetadataFile->setText(settingsContainer.value("filenameMetadata").toString());
    ui->lineEditFilelistingFile->setText(settingsContainer.value("filenameFilelisting").toString());
    ui->lineEditLogPath->setText(settingsContainer.value("logPath").toString());
    ui->checkBoxDeleteLog->setChecked(settingsContainer.value("deleteLog").toBool());
    ui->checkBoxLoadProposalInfo->setChecked(settingsContainer.value("loadProposalInfo").toBool());
    ui->checkBoxSaveProposalInfo->setChecked(settingsContainer.value("saveProposalInfo").toBool());
    ui->checkBoxDisableEmails->setChecked(settingsContainer.value("disableEmails").toBool());
    ui->checkBoxSaveLog->setChecked(settingsContainer.value("saveLog").toBool());
    ui->checkBoxEnableArchive->setChecked(settingsContainer.value("enableArchive").toBool());
    ui->checkBoxEnableIngest->setChecked(settingsContainer.value("enableIngest").toBool());
    ui->checkBoxUsePgroupAsPath->setChecked(settingsContainer.value("usePgroupAsDefaultPath").toBool());
    ui->checkBoxInvestigatorIsPI->setChecked(settingsContainer.value("investigatorIsPI").toBool());
    ui->checkBoxGuessDatasetname->setChecked(settingsContainer.value("guessDatasetname").toBool());
    ui->checkBoxPrintCalls->setChecked(settingsContainer.value("printIngestion").toBool());
    ui->checkBoxSystemCalls->setChecked(settingsContainer.value("disableSystemCalls").toBool());
    ui->comboBoxServer->setCurrentIndex(settingsContainer.value("serverID").toInt());
    updateServerURL(settingsContainer.value("serverID").toInt());

    // not implemented:
    ui->checkBoxDisableEmails->setVisible(false);
}

void Settings::reset(){
    ui->lineEditMetadataFile->setText(baseUi->defaultSettings.filenameMetadata);
    ui->lineEditFilelistingFile->setText(baseUi->defaultSettings.filenameFilelisting);
    ui->lineEditLogPath->setText(baseUi->defaultSettings.logPath);
    ui->checkBoxDeleteLog->setChecked(baseUi->defaultSettings.deleteLog);
    ui->checkBoxLoadProposalInfo->setChecked(baseUi->defaultSettings.loadProposalInfo);
    ui->checkBoxSaveProposalInfo->setChecked(baseUi->defaultSettings.saveProposalInfo);
    ui->checkBoxDisableEmails->setChecked(baseUi->defaultSettings.disableEmails);
    ui->checkBoxSaveLog->setChecked(baseUi->defaultSettings.saveLog);
    ui->checkBoxEnableArchive->setChecked(baseUi->defaultSettings.enableArchive);
    ui->checkBoxEnableIngest->setChecked(baseUi->defaultSettings.enableIngest);
    ui->checkBoxUsePgroupAsPath->setChecked(baseUi->defaultSettings.usePgroupAsDefaultPath);
    ui->checkBoxInvestigatorIsPI->setChecked(baseUi->defaultSettings.investigatorIsPI);
    ui->checkBoxGuessDatasetname->setChecked(baseUi->defaultSettings.guessDatasetname);
    ui->checkBoxPrintCalls->setChecked(baseUi->defaultSettings.printIngestion);
    ui->checkBoxSystemCalls->setChecked(baseUi->defaultSettings.disableSystemCalls);
    ui->comboBoxServer->setCurrentIndex(baseUi->defaultSettings.serverID);
    updateServerURL(baseUi->defaultSettings.serverID);
}

void Settings::logFileBrowser(){
    auto fileName = QFileDialog::getExistingDirectory(this,tr("Select log file path"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (fileName.length()>0){
        ui->lineEditLogPath->setText(fileName);
    }
}

void Settings::updateServerURL(int val){
    ui->lineEditServer->setText(SciCat::DataCatalogue::serverList()[val]);
}

void Settings::acceptChanges(){
    // compare with the server in use, not the stored one
    const bool serverChanged = baseUi->scicat->getServerID()!=ui->comboBoxServer->currentIndex();
    updateSettings();
    // before login the caller applies the new server directly, no restart needed
    if (serverChanged && context==Context::AfterLogin){
        QMessageBox msgBox;
        msgBox.setText("You must restart the application to apply these changes.");
        msgBox.exec();
    }
    accept();
}

void Settings::updateSettings(){
    // update settingsContainer
    QSettings settingsContainer("PSI", "SciCat");
    settingsContainer.setValue("filenameFilelisting", ui->lineEditFilelistingFile->text());
    settingsContainer.setValue("filenameMetadata", ui->lineEditMetadataFile->text());
    settingsContainer.setValue("logPath", ui->lineEditLogPath->text());
    settingsContainer.setValue("deleteLog", (bool) ui->checkBoxDeleteLog->checkState());
    settingsContainer.setValue("loadProposalInfo", (bool) ui->checkBoxLoadProposalInfo->checkState());
    settingsContainer.setValue("saveProposalInfo", (bool) ui->checkBoxSaveProposalInfo->checkState());
    settingsContainer.setValue("disableEmails", (bool) ui->checkBoxDisableEmails->checkState());
    settingsContainer.setValue("saveLog", (bool) ui->checkBoxSaveLog->checkState());
    settingsContainer.setValue("enableArchive", (bool) ui->checkBoxEnableArchive->checkState());
    settingsContainer.setValue("enableIngest", (bool) ui->checkBoxEnableIngest->checkState());
    settingsContainer.setValue("usePgroupAsDefaultPath", (bool) ui->checkBoxUsePgroupAsPath->checkState());
    settingsContainer.setValue("investigatorIsPI", (bool) ui->checkBoxInvestigatorIsPI->checkState());
    settingsContainer.setValue("guessDatasetname", (bool) ui->checkBoxGuessDatasetname->checkState());
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

void Settings::checkMetadataFile(){
    if (ui->lineEditMetadataFile->text().length()==0){
        qDebug() << "is empty";
        ui->lineEditMetadataFile->setText(baseUi->defaultSettings.filenameMetadata);
    } else {
        const auto text = ui->lineEditMetadataFile->text();
        if (!text.endsWith(".json", Qt::CaseInsensitive)){
            ui->lineEditMetadataFile->setText(ui->lineEditMetadataFile->text() + ".json");
        }
    }

}

void Settings::checkFilelistingFile(){
    if (ui->lineEditFilelistingFile->text().length()==0){
        ui->lineEditFilelistingFile->setText(baseUi->defaultSettings.filenameFilelisting);
    } else {
        const auto text = ui->lineEditFilelistingFile->text();
        if (!text.endsWith(".txt", Qt::CaseInsensitive)){
            ui->lineEditFilelistingFile->setText(ui->lineEditFilelistingFile->text() + ".txt");
        }
    }
}


Settings::~Settings()
{
    delete ui;
}
