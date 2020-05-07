#include "settings.h"

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

    // get parent class instance
    baseUi = dynamic_cast<Dialog*>(parent);

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


Settings::~Settings()
{
    delete ui;
}
