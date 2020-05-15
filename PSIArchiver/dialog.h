#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QTableView>
#include <iostream>
#include <vector>
#include <string>
#include <QFileSystemModel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QMovie>
#include <QThread>
#include <QCompleter>
#include <QJsonDocument>
#include <QFile>
#include <QJsonArray>
#include <QLabel>
#include <QSequentialAnimationGroup>
#include <QBitmap>
#include <QDateTime>
#include <QPushButton>
#include <QSettings>
#include <QCoreApplication>
#include <QProcess>
#include <QVersionNumber>
#include "qfiledialog.h"
#include "QMessageBox"

// forward declaration for external header files
class DatasetModel;
class Ingestor;
class Dialog;
class LogBrowser;

#include "archiverutils.h"
#include "datasetmodel.h"
#include "logbrowser.h"
#include "ui_dialog.h"
#include "ui_datasetbrowser.h"
#include "filebrowser.h"
#include "settings.h"
#include "datasetbrowser.h"




struct styleSheets_t {
    QString buttonNavigation;
    QString buttonNotActive;
};

struct defaultSettings_t {
    QString binaryPath = "datasetIngestor";
    QString datasetRetriever = "datasetRetriever";
    QString logPath = QDir::homePath() + "/.scicat/log";
    QString filenameFilelisting = "filelisting.txt";
    QString filenameMetadata = "metadata.json";
    bool deleteLog = false;
    QStringList serverList = {"https://dacat.psi.ch/api/v3/", "https://dacat-qa.psi.ch/api/v3/", "https://dacat-development.psi.ch/api/v3/"};
    QStringList serverListAuth = {"https://dacat.psi.ch/auth/msad", "https://dacat-qa.psi.ch/auth/msad", "https://dacat-development.psi.ch/auth/msad"};
    QStringList serverListNames = {"Production", "QA", "Development"};
    QStringList creationLocationList = {"/PSI/SLS/ADRESS-RIXS",
                                        "/PSI/SLS/ADRESS-SX-ARPES",
                                        "/PSI/SLS/CSAXS",
                                        "/PSI/SLS/MICRO-XAS",
                                        "/PSI/SLS/MICRO-XAS-FEMTO",
                                        "/PSI/SLS/MS-POWDER",
                                        "/PSI/SLS/MS-SURF-DIFFR",
                                        "/PSI/SLS/NANOXAS",
                                        "/PSI/SLS/PEARL",
                                        "/PSI/SLS/PHOENIX",
                                        "/PSI/SLS/POLLUX",
                                        "/PSI/SLS/MX",
                                        "/PSI/SLS/SIM",
                                        "/PSI/SLS/SIS-COPHEE",
                                        "/PSI/SLS/SIS-HRPES",
                                        "/PSI/SLS/SUPER-XAS",
                                        "/PSI/SLS/TOMCAT",
                                        "/PSI/SLS/VUV",
                                        "/PSI/SLS/XIL-II",
                                        "/PSI/SLS/XTREME",
                                        "/PSI/SWISSFEL/ARAMIS-ALVRA",
                                        "/PSI/SWISSFEL/ARAMIS-BERNINA",
                                        "/PSI/SWISSFEL/ARAMIS-CRISTALLINA",
                                        "/PSI/SINQ/AMOR",
                                        "/PSI/SINQ/DMC",
                                        "/PSI/SINQ/EIGER",
                                        "/PSI/SINQ/FOCUS",
                                        "/PSI/SINQ/HRPT",
                                        "/PSI/SINQ/ICON",
                                        "/PSI/SINQ/MORPHEUS",
                                        "/PSI/SINQ/NARZISS",
                                        "/PSI/SINQ/NEUTRA",
                                        "/PSI/SINQ/POLDI",
                                        "/PSI/SINQ/RITA-II",
                                        "/PSI/SINQ/SANS-I",
                                        "/PSI/SINQ/SANS-II",
                                        "/PSI/SINQ/TASP",
                                        "/PSI/SINQ/ZEBRA",
                                        "/PSI/SMUS/DOLLY",
                                        "/PSI/SMUS/GPD",
                                        "/PSI/SMUS/GPS",
                                        "/PSI/SMUS/HAL-9500",
                                        "/PSI/SMUS/LEM",
                                        "/PSI/SMUS/LTF"};
    uint serverID = 0;
    bool printIngestion = true;
    bool disableSystemCalls = false;
    bool usePgroupAsDefaultPath = true;
    bool saveProposalInfo = true;
    bool loadProposalInfo = true;
    bool disableEmails = false;
    bool saveLog = true;
    bool enableArchive = true;
    bool enableIngest = true;
    bool investigatorIsPI = false;
    bool requestAlreadyRetrievedData = true;
    bool guessDatasetname = true;



};

struct inputDatasets_t {
    bool inputDatasetTabIsVisible = true;
    QWidget *tab = nullptr;
//    QIndex tabindex;
    QString tabLabel;
};

struct retrieveContainer_t {
    QString pid;
    QString name;
    QString retrieved = "false";
    QString retrievable;
};

struct retrieveJobsContainer_t {
    QString name;
    QString outdir;
    QString pid;
    QProcess* process;
};

inline QDebug operator<<(QDebug out, const retrieveContainer_t& container)
{
    out << container.pid << " " << container.name << " " << container.retrievable << " " << container.retrieved;
    return out;
}


inline void updateSettingsContainer(QSettings &settingsContainer, const QString &key, const QVariant &value){
    if (!settingsContainer.contains(key)){
        settingsContainer.setValue(key, value);
    }
}



struct metadataContainer_t {
    std::string PIemail;
    std::string PIFirstname;
    std::string PILastname;
    std::string proposalID;
    std::string proposalTitle;
    std::string proposalAbstract;
    std::string instrument;
    std::string pgroup;
    std::string type;
    QString datasetName;
    std::string sourceFolder;
    QString creationLocation;
    std::string inputDatasets;
    QString usedSoftware;
    std::string keywords;
    std::string samplename;
    std::string sampleDescription;
    std::string ownerGroup;
    std::string owner;
    std::string ownerEmail;
    QString investigator;
    QString description;
    QString userMail;
    QString username;
    QString pass;
    std::string userInfo;
    bool isBeamlineAccount = false;
    QString metadataFilename;
    QString filelistingFilename;
    QString ingestionLogDir;
    bool retrievedPIInfoSuccessfully;
};

class ButtonHoverWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ButtonHoverWatcher(QObject * parent = Q_NULLPTR);
    virtual bool eventFilter(QObject * watched, QEvent * event) Q_DECL_OVERRIDE;
    void setIcons(std::vector<std::string> icons);
    void setButtonIsActive(bool val);
private:
    std::vector<QIcon> iconsVector;
    bool buttonIsActive=false;
signals:
    void buttonIsActiveStatus(int value);

};


class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    Ui::Dialog *ui;
    int numSlides, progressbarStep;
    QStringList openFileBrowser();
    std::string token;
    void keyPressEvent(QKeyEvent *event);
    std::vector<QStringList> datasetContainer;
    DatasetModel* datasets_m = new DatasetModel();
    void sendWarning(QString warningMsg);
    QMovie *movie = new QMovie(":/images/ajax-loader.gif");
    void getAvailPgroups();
    metadataContainer_t metadataContainer;
    std::vector<QString> filelistingContainer;
    QLabel *labelSlider;
    void toggleNavigationBar(int section);
    defaultSettings_t defaultSettings;
    void updateRetrieveList();
    int secID=-1;
    void updateNavigationIcons(int targetID);
    ButtonHoverWatcher *pgroupWatcher = new ButtonHoverWatcher;
    ButtonHoverWatcher *archiveWatcher = new ButtonHoverWatcher;
    ButtonHoverWatcher *retrieveWatcher = new ButtonHoverWatcher;
//    void json2TreeModel(QTreeWidgetItem *item, QJsonObject &inObj);



public slots:
    void reject();


private slots:
    void nextSlide();
    void fileDialogFilelisting();
    void clearAllFilelisting();
    void clearFilelisting();
    void clearAllDatasets();
    void clearDatasets();
    void getPI();
    void goToPgroup();
    void goToFiles();
    void goToMetadata();
    void goToSubmit();
    void openDatasetBrowser();
    void comboBoxDatatypeChanged(int val);
    void appendMetadataEntry();
    void removeMetadataEntry();
    void piChanged();
    void ingestionFinished(int exitCode);
    void restart();
    void datasetNameCheck();
    void investigatorChanged();
    void descriptionChanged();
    void usedSoftwareChanged();
    void goToArchive();
    void goToRetrieve();
    void exportMetadata();
    void importMetadata();
    void openSettings();
    void readIngestionOutput();
    void ingestionFailed(QProcess::ProcessError error);
    void openIngestionLog();
    void creationLocationChanged(int val);
    void requestRetrieve();
    void saveRetrievedData();
    void retrieverFailed(QProcess::ProcessError error);
    void retrieverFinished(int val);
    void goToRequest();
    void goToJobs();
    void killJob();
    void detachJob();
    void updateJobTable();






private:
    std::vector<void (Dialog::*)()> nextSlideButtonFunctions;
    void PINextSlide();
    void filelistingNextSlide();
    void metadataNextSlide();
    void submitNextSlide();
    void goToNextSlide();
    int currentSlideIndex;
    bool checkPgroup();
    bool checkPIInfo();
    void updateNavigationButton();
    void goToPage(int pageIndex);
    bool filelistingCheckAndUpdate();
    bool checkMetadata();
    void addEntryMetadataTreeView(QString name, QString description, QTreeWidgetItem *parent, bool setFocus=false, int flag=2);
    void addEntryMetadataTreeView(QString name, QString description, QString parent, bool setFocus=false, int flag=2);
    void addEntryMetadataTreeView(QString name, QString description);
    styleSheets_t styleSheetsContainer;
    std::vector<QPushButton *> navigationButtons;
    void updateMetadataEntry(QString name, QString targetValue, int flag=2);
    void startIngestion();
    void setButtonsEnabled(bool enabled);
    bool PIHasChanged = true;
    bool PIIsValid = false;
    bool pgroupIsValid = false;
    void resetContent();
    void quit();
    QStringList availPgroups;
    QCompleter *pgroupCompleter;
    std::string savePath = "~/scicat/log/";
    void updateItemFlag(QTreeWidgetItem *item, int flag);
    std::vector<std::string> loginIcons;
    std::vector<std::string> archiveIcons;
    std::vector<std::string> retrieveIcons;
    inputDatasets_t inputDatasetsContainer;
    int numPagesArchive = 3;
    int numPagesRetrieve = 1;
    int numPagesLogin = 1;
    bool ingested = false;
    QProcess *ingestion;
    void prepareIngestion();
    LogBrowser *ingestionLog;
    std::vector<retrieveContainer_t> retrieveContainer;
    std::vector<retrieveJobsContainer_t> retrieverList;
    void startRetriever(const QString& pid, const QString& outdir, const QString& name);
    void updateRetrieveJobs();


};


QString extractSourceFolder(std::vector<QString> container);
QJsonObject treeModel2Json(QTreeWidgetItem *item);
void json2TreeModel(QJsonObject &inObj, QTreeWidgetItem *item, QTreeWidget* parentWidget=nullptr);
//void json2TreeModel(QJsonObject &inObj, QTreeWidget* widget);
int countFilesAndDirRecursive(QString dirName);


#endif // DIALOG_H



