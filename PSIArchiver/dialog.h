#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QTableView>
#include <iostream>
#include <vector>
#include <string>
#include <QFileSystemModel>
#include "archiverutils.h"
#include <QAbstractTableModel>
#include <QPainter>
#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Dialog;
    class fileBrowser;
    class Login;
}
QT_END_NAMESPACE

class DatasetModel;


struct styleSheets_t {
    QString buttonNavigation;
    QString buttonNotActive;
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
    std::string datasetName;
    std::string sourceFolder;
    std::string creationLocation;
    std::string inputDatasets;
    std::string usedSoftware;
    std::string keywords;
    std::string samplename;
    std::string sampleDescription;
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    Ui::Dialog *ui;
    int numSlides, progressbarStep;
    std::vector<QString> filelistingContainer;
    QStringList openFileBrowser();
    void updateFilelistingDisplay();
    std::string token;
    void keyPressEvent(QKeyEvent *event);
    std::vector<QStringList> datasetContainer;


private slots:
    void nextSlide();
    void fileDialogFilelisting();
    void clearAllFilelisting();
    void clearFilelisting();
    void getPI();
    void goToPgroup();
    void goToFiles();
    void goToMetadata();
    void goToSubmit();



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
    metadataContainer_t metadataContainer;
    styleSheets_t styleSheetsContainer;
    std::vector<QPushButton *> navigationButtons;




};

class fileBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit fileBrowser(QWidget *parent = nullptr);
    ~fileBrowser();
    void connectToUi(Ui::Dialog *ui);
    bool removeDuplicates = true;
//    static void updateFileCount(Ui::Dialog *baseUi);
//    static int countFilelistingEntries(QString path);

private slots:
    void getCurrentEntry();
    void setParentDirActive();
    void setChildDirActive();
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

#endif // DIALOG_H


class DatasetModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatasetModel(const std::vector<QStringList>& datasetContainer, QObject *parent = nullptr) :
        QAbstractTableModel(parent), datasetContainerModel(datasetContainer){}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
//    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
//    const QStringList& datasetContainerModel;
    std::vector<QStringList> datasetContainerModel;
};
