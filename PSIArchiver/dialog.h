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

QT_BEGIN_NAMESPACE
namespace Ui {
    class Dialog;
    class fileBrowser;
    class Login;
}
QT_END_NAMESPACE

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

private slots:
    void nextSlide();
    void previousSlide();
    void fileDialogFilelisting();
    void clearAllFilelisting();
    void clearFilelisting();
    void getPI();

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
    metadataContainer_t metadataContainer;




};

class fileBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit fileBrowser(QWidget *parent = nullptr);
    ~fileBrowser();
    void connectToUi(Ui::Dialog *ui);
    bool removeDuplicates = true;
    static void updateFileCount(Ui::Dialog *baseUi);
    static int countFilelistingEntries(QString path);

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
