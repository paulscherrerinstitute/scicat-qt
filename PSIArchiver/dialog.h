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


#include "archiverutils.h"
#include "datasetmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class Dialog;
    class fileBrowser;
    class Login;
    class DatasetBrowser;
}
QT_END_NAMESPACE

class DatasetModel;
class Ingestor;

struct styleSheets_t {
    QString buttonNavigation;
    QString buttonNotActive;
};

struct defaultSettings_t {
    bool investigatorIsPI = false;
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
    std::string creationLocation;
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
};

struct ingestionIO_t {
    QString binaryPath = "datasetIngestor";
    QString logPath = QDir::homePath() + "/.scicat/log/";
    QString filenameFilelisting = "filelisting.txt";
    QString filenameMetadata = "metadata.json";
    bool deleteLog = false;
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
    ingestionIO_t ingestionContainer;
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
    void json2TreeModel(QTreeWidgetItem *item, QJsonObject &inObj);



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
    void ingestionFinished();
    void restart();
    void datasetNameCheck();
    void investigatorChanged();
    void descriptionChanged();
    void usedSoftwareChanged();
    void goToArchive();
    void goToRetrieve();
    void exportMetadata();
    void importMetadata();


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
    Ingestor *ingestor_m;
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

    







};

class fileBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit fileBrowser(QWidget *parent = nullptr);
    ~fileBrowser();
    void connectToUi(Ui::Dialog *ui);
    bool removeDuplicates = true;

private slots:
    void getCurrentEntry();
    void setParentDirActive();
    void selectButton();


private:
    Ui::fileBrowser *uiFileBrowser;
    QFileSystemModel *fileModel;
    QStringList filePathQList;
    std::string filePath;
    Ui::Dialog *baseUi;

    void appendEntry();





};

class Login : public QDialog
{
    Q_OBJECT
public:
    Login(QWidget *parent = nullptr);
    ~Login();
    bool accessGranted = false;
    void connectToGui(Dialog *ui);
private:
    Ui::Login *uiLogin;
    Dialog *baseUi;
private slots:
    void getLoginData();


signals:


};

class DatasetBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit DatasetBrowser(const std::vector<QStringList>& datasetContainer, QWidget *parent = nullptr);
    ~DatasetBrowser();
    void connectToUi(Dialog *uiDialog);
private slots:
    void selectButton();
private:
    Ui::DatasetBrowser *ui;
    Dialog *baseUi;
    DatasetModel* datasets_m = new DatasetModel();
};

class Ingestor : public QThread
{
  Q_OBJECT
public:
    Ingestor(){};
    Ingestor(std::string commandIn);
    void setCommand(std::string commandIn);
    void prepareIngestion();
    void connectToUi(Dialog *uiDialog);
protected:
    void run();
private:
    std::string command;
    Dialog *baseUi;
};

QString extractSourceFolder(std::vector<QString> container);
QJsonObject treeModel2Json(QTreeWidgetItem *item);

#endif // DIALOG_H



