#ifndef DIALOG_H
#define DIALOG_H


// forward declaration for external header files
class DatasetModel;
class Ingestor;
class Dialog;
class LogBrowser;

#include "pch.h"
#include "datasetmodel.h"
#include "logbrowser.h"
#include "ui_dialog.h"
#include "ui_datasetbrowser.h"
#include "filebrowser.h"
#include "settings.h"
#include "datasetbrowser.h"
#include "filedialogfilelisting.h"
#include "networkmanager.h"
#include "creationlocationcreator.h"
#include "datacatalogueClasses.h"
#include "datacatalogue.h"
#include "datasetingestor.h"
#include <unordered_map>



#define qDebugLine(X) (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug() << "[" <<  X << "]: ")
#define QT_NO_QDEBUGLINE_MACRO(X) while (false) QMessageLogger().noDebug()

#ifdef QT_NO_DEBUG_OUTPUT
#undef qDebugLine
#define qDebugLine(X) QT_NO_QDEBUGLINE_MACRO(X)
#endif

struct fileCounter_t {
    int numFiles;
    QString name;
    QList<fileCounter_t> subDir;
};

struct styleSheets_t {
    QString buttonNavigation;
    QString buttonNotActive;
};

struct defaultSettings_t {
    QString datasetIngestorBinary = "datasetIngestor";
    QString datasetRetrieverBinary = "datasetRetrieverBinary";
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
                                        "/PSI/SMUS/LTF",
                                        "/PSI/RA",
                                        "/PSI/MERLIN",
                                        "custom"};
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
    int maxFileNum = 150000;



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
    std::shared_ptr<SciCat::DatasetRetriever> process;
};

inline QDebug operator<<(QDebug out, const retrieveContainer_t& container)
{
    out << container.pid << " " << container.name << " " << container.retrievable << " " << container.retrieved;
    return out;
}
inline QDebug operator<<(QDebug out, const std::string& str)
{
    out << QString::fromStdString(str);
    return out;
}

inline QDebug operator<<(QDebug out, const SciCat::authenticationStateAccount& auth){
    switch (auth){
        case SciCat::authenticationStateAccount::ValidUser: out << "<authenticationStateAccount: ValidUser>"; return out;
        case SciCat::authenticationStateAccount::ValidBLAccount: out << "<authenticationStateAccount: ValidBLAccount>"; return out;
        case SciCat::authenticationStateAccount::Invalid: out << "<authenticationStateAccount: Invalid>"; return out;
        case SciCat::authenticationStateAccount::ConnectionError: out << "<authenticationStateAccount: ConnectionError>"; return out;

    }
}

inline void updateSettingsContainer(QSettings &settingsContainer, const QString &key, const QVariant &value){
    if (!settingsContainer.contains(key)){
        settingsContainer.setValue(key, value);
    }
}



struct metadataContainer_t {
    QString ingestionLogDir;
    QString metadataFilename;
    QString filelistingFilename;

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
    void sendWarning(QString warningMsg);
    QMovie *movie = new QMovie(":/images/ajax-loader.gif");
    void setAvailPgroups();
    metadataContainer_t metadataContainer;
    std::vector<QString> filelistingContainer;
    QLabel *labelSlider;
    void toggleNavigationBar(int section);
    defaultSettings_t defaultSettings;
    void updateRetrieveList();
    int secID=-1;
    void updateNavigationIcons(int targetID);
    SciCat::DataCatalogue* scicat = nullptr;
    ButtonHoverWatcher *pgroupWatcher;
    ButtonHoverWatcher *archiveWatcher;
    ButtonHoverWatcher *retrieveWatcher;



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
    void ingestionFailed(QProcess::ProcessError error);
    void openIngestionLog();
    void creationLocationChanged(int val);
    void requestRetrieve();
    void saveRetrievedData();
    void goToRequest();
    void goToJobs();
    void killJob();
    void detachJob();
    void updateJobTable();
    void fileCounterFinished(bool force=false);
    void updateRetrieverFiles();
    void toggleFilesMetadata();






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
    void setButtonsEnabled(bool enabled);
    bool PIHasChanged = true;
    bool pgroupIsValid = false;
    void resetContent();
    void quit();
    QStringList availPgroups;
    QCompleter *pgroupCompleter=nullptr;
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
    QProcess *ingestion=nullptr;
    LogBrowser *ingestionLog=nullptr;
    std::vector<retrieveContainer_t> retrieveContainer;
    std::vector<retrieveJobsContainer_t> retrieverList;
    void startRetriever(const QString& pid, const QString& outdir, const QString& name);
    void updateRetrieveJobs();
    QFuture<int> futureGetFileNum;
    QFutureWatcher<int> watcherFileCounter;
    bool startIngestionWhenReady = false;
    bool ingestionIsRunning = false;
    std::unordered_map<std::string, QJsonDocument> retrieveDataMap;
    void sendMessage(QString msg);
    bool resizeRetrieverFiles = true;
};


QString extractSourceFolder(std::vector<QString> container);
QJsonObject treeModel2Json(QTreeWidgetItem *item);
void json2TreeModel(QJsonObject &inObj, QTreeWidgetItem *item, QTreeWidget* parentWidget=nullptr);
fileCounter_t countFilesAndDirRecursive(QString dirName);
int getFileNum(std::vector<QString> container);


#endif // DIALOG_H



