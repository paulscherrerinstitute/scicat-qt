#include "dialog.h"
#include "./ui_dialog.h"
#include "ui_filebrowser.h"
#include "qfiledialog.h"
#include "QMessageBox"
#include "ui_login.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{

    ui->setupUi(this);
    int indx;
    indx = ui->stackedWidget->indexOf(ui->stackedWidget->currentWidget());

//    ui->nextSlideButton->setFlat(true);
//    ui->nextSlideButton-->setS;

    // update number of slides/pages
    numSlides = ui->stackedWidget->count();
    progressbarStep = (int) (100/(numSlides-1));

    // connect button signals to slots
    connect(ui->nextSlideButton, SIGNAL (clicked()), this, SLOT (nextSlide()));
    connect(ui->previousSlideButton, SIGNAL (clicked()), this, SLOT (previousSlide()));
    connect(ui->addFilesDirectoriesButton, SIGNAL (clicked()), this, SLOT (fileDialogFilelisting()));
    connect(ui->clearButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearFilelisting()));
    connect(ui->clearAllButtonFilelisting, SIGNAL (clicked()), this, SLOT (clearAllFilelisting()));
    connect(ui->getPIButton, SIGNAL (clicked()), this, SLOT (getPI()));
    connect(ui->lineEditPgroup, SIGNAL (editingFinished()), this, SLOT (getPI()));

    // prepare function handles for nextSlideButton
    nextSlideButtonFunctions.push_back(&Dialog::PINextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::filelistingNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::metadataNextSlide);
    nextSlideButtonFunctions.push_back(&Dialog::submitNextSlide);

    //
    ui->lineEditPIMail->setEnabled(false);
    ui->lineEditPIFirstname->setEnabled(false);
    ui->lineEditPILastname->setEnabled(false);
    ui->lineEditProposalID_2->setEnabled(false);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());




}

void Dialog::keyPressEvent(QKeyEvent *event){
    if (event->key() != Qt::Key_Escape){
        QDialog::keyPressEvent(event);
    }
}

void Dialog::PINextSlide(){
    std::cout << "PI" << std::endl;
    if (checkPIInfo()){
        goToNextSlide();
    }
}
void Dialog::filelistingNextSlide(){
    if (ui->filelistingListWidget->count()>0){
        goToNextSlide();
    } else {
        QMessageBox::information(this, tr("Invalid entry"), tr("Please add at least one file/directory.") );
    }

    // prepare metadata entries
    ui->lineEditOwnerEmail->setText(QString::fromStdString(metadataContainer.PIemail));
    ui->lineEditOwner->setText(QString::fromStdString(metadataContainer.PIFirstname + " " + metadataContainer.PILastname));
    ui->lineEditOwnerGroup->setText(QString::fromStdString(metadataContainer.pgroup));
    ui->lineEditProposalID->setText(QString::fromStdString(metadataContainer.proposalID));
//    curl -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%20%22p17301%22%7D%2C%20%22fields%22%3A%20%7B%22datasetName%22%3Atrue%2C%20%22pid%22%3Atrue%7D%7D&access_token=$SCICAT_TOKEN'
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Datasets?filter=%7B%22where%22%3A%7B%22ownerGroup%22%3A%20%22" + metadataContainer.pgroup + "%22%7D%2C%20%22fields%22%3A%20%7B%22datasetName%22%3Atrue%2C%20%22pid%22%3Atrue%7D%7D&access_token=" + token + "'");
    auto delim = '}';
    std::vector<std::string> tmpVec = utils::split(tmp, delim);
//    QAbstractItemModel datasetsModel;
//    datasetsModel
//    QTableView datasets;
//    datasets.setModel()
//    for (auto v:tmpVec){
//        std::cout << v << std::endl;
//    }
//    std::cout << tmp << std::endl;
}
void Dialog::metadataNextSlide(){
    std::cout << "metadata" << std::endl;
    goToNextSlide();
}
void Dialog::submitNextSlide(){
    std::cout << "submit" << std::endl;
    goToNextSlide();
}


void Dialog::nextSlide(){
    // update currentSlideIndex
    currentSlideIndex = ui->stackedWidget->indexOf(ui->stackedWidget->currentWidget());
    (this->*nextSlideButtonFunctions[currentSlideIndex])();
}

void Dialog::goToNextSlide(){
    ui->stackedWidget->setCurrentIndex(++currentSlideIndex);
    ui->progressBar->setValue(progressbarStep*(currentSlideIndex));
}

void Dialog::previousSlide(){
    // update currentSlideIndex
    currentSlideIndex = ui->stackedWidget->indexOf(ui->stackedWidget->currentWidget());
    ui->stackedWidget->setCurrentIndex(--currentSlideIndex);
    ui->progressBar->setValue(progressbarStep*(currentSlideIndex));
}

bool Dialog::checkPgroup(){
    bool pgroupIsValid = true;
    if (ui->lineEditPgroup->text().size()==0){
        QMessageBox::information(this, tr("Error"), tr("pgroup cannot be empty") );
        pgroupIsValid = false;
    } else if ((ui->lineEditPgroup->text().size() != 6) || (ui->lineEditPgroup->text().toStdString().find("p") != 0)){
        QMessageBox::information(this, tr("Error"), tr("Please enter a valid pgroup") );
        pgroupIsValid = false;
    }

    // check if user has access to the given pgroup
    auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22fields%22%3A%20%22ownerGroup%22%7D&access_token=" + token + "'");
    if (pgroupIsValid && (tmp.find(ui->lineEditPgroup->text().toStdString())==std::string::npos)){
        auto msg = "Cannot access pgroup " + ui->lineEditPgroup->text().toStdString() + ". Permission denied.";
        QMessageBox::information(this, tr("Error"), tr(msg.c_str()));
        pgroupIsValid = false;
    }

    return pgroupIsValid;
}

bool Dialog::checkPIInfo(){
    bool infoIsValid = true;



    return infoIsValid;

}

void Dialog::getPI(){
    if (Dialog::checkPgroup()){
        metadataContainer.pgroup = ui->lineEditPgroup->text().toStdString();
        auto tmp = utils::system_call("curl -s -X GET --header 'Accept: application/json' 'https://dacat.psi.ch/api/v3/Proposals?filter=%7B%22where%22%3A%20%7B%22ownerGroup%22%3A%22" + metadataContainer.pgroup + "%22%7D%7D&access_token=" + token + "'");
//        std::cout << tmp << std::endl;
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


        }

        ui->lineEditPIMail->setEnabled(true);
        ui->lineEditPIFirstname->setEnabled(true);
        ui->lineEditPILastname->setEnabled(true);
        ui->lineEditProposalID_2->setEnabled(true);
    }
}

void Dialog::fileDialogFilelisting(){
    fileBrowser fb(this);
    fb.connectToUi(ui);
    fb.exec();

    ui->filelistingListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void Dialog::clearFilelisting(){
    // get current item
    std::cout << ui->filelistingListWidget->currentItem()->text().toStdString() << std::endl;
    ui->filelistingListWidget->removeItemWidget(ui->filelistingListWidget->currentItem());

    ui->filelistingListWidget->setUpdatesEnabled(false);


    QModelIndexList indexes;
    while((indexes = ui->filelistingListWidget->selectionModel()->selectedIndexes()).size()) {
        ui->filelistingListWidget->model()->removeRow(indexes.first().row());
    }
    ui->filelistingListWidget->setUpdatesEnabled(true);

//    fileBrowser::updateFileCount(ui);
}

void Dialog::clearAllFilelisting(){
    ui->filelistingListWidget->clear();
//    fileBrowser::updateFileCount(ui);
}

void Dialog::updateFilelistingDisplay(){
    ui->filelistingListWidget->clear();
    for (uint containerIndex=0; containerIndex<filelistingContainer.size(); containerIndex++){
        ui->filelistingListWidget->addItem(filelistingContainer[containerIndex]);
    }

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
//    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),"~/", QFileDialog::Directory);
    return fileName;
}

Dialog::~Dialog()
{
    delete ui;
}





fileBrowser::fileBrowser(QWidget *parent) :
    QDialog(parent),
    uiFileBrowser(new Ui::fileBrowser)
{
    uiFileBrowser->setupUi(this);

    QString sPath = "/";
    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(sPath);
    uiFileBrowser->columnView->setModel(fileModel);
    uiFileBrowser->columnView->setRootIndex(fileModel->index("/"));
    uiFileBrowser->columnView->selectionModel()->setCurrentIndex(fileModel->index("/home/wakonig_k"), QItemSelectionModel::Select);
    uiFileBrowser->columnView->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    uiFileBrowser->columnView->show();

    connect(uiFileBrowser->pushButtonFileBrowser, SIGNAL (clicked()), this, SLOT (selectButton()));
//    connect(uiFileBrowser->columnView->selectionModel(), SIGNAL (doubleClicked()), this, SLOT (setChildDirActive()));
    connect(uiFileBrowser->toolButtonUp, SIGNAL (clicked()), this, SLOT (setParentDirActive()));
    uiFileBrowser->columnView->selectionModel()->clearSelection();
}

void fileBrowser::connectToUi(Ui::Dialog *ui){
    baseUi = ui;
}

void fileBrowser::selectButton(){
    getCurrentEntry();
    appendEntry();
//    updateFileCount(baseUi);

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
    std::cout << indexList.size() << std::endl;
    for (int ii=0; ii<indexList.size(); ++ii) {
        filePath = "/" + indexList[ii].data().toString().toStdString();
        QModelIndex parentIndex, parentIndexOld;
        QVariant dataParent;
        QString dataParentString;
        parentIndexOld = index;
        std::string parentStdString;
        std::cout << filePath << std::endl;

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


void fileBrowser::setChildDirActive(){
    std::cout << "Child" << std::endl;

}


int fileBrowser::countFilelistingEntries(QString path){
    QDirIterator it(path , QDirIterator::Subdirectories); //QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot
     if (it.hasNext()){
         std::cout << "here" << std::endl;
         std::cout << (it.fileName()).toStdString() << std::endl;
     }
    QDir dir(path);
    dir.setFilter( QDir::AllEntries | QDir::NoDotAndDotDot );
    int total_files = dir.count();
    std::cout << total_files << std::endl;
    return total_files;
}

void fileBrowser::updateFileCount(Ui::Dialog *baseUi){
    int counter = 0;
    for (int ii=0; ii<baseUi->filelistingListWidget->count(); ii++){
        counter += countFilelistingEntries(baseUi->filelistingListWidget->item(ii)->text());
    }
    baseUi->filelistingCounter->setText("Files: " + QString::number(counter));
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
    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();
    std::string token;
    auto tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' 'https://dacat.psi.ch/api/v3/Users/login'");
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        QMessageBox::information(this, tr("Authentication error"), tr("Please check your username and password") );
        uiLogin->lineEditPassword->clear();
    } else {
        accessGranted = true;
        if (tmp.find("access_token") != std::string::npos){
            token = utils::extractFromJson(tmp, "access_token");
        } else if (tmp.find("id") != std::string::npos) {
            token = utils::extractFromJson(tmp, "id");
        } else {
            std::cout << tmp << std::endl;
            QMessageBox::information(this, tr("Connection error"), tr("Could not connect to SciCat") );
            accessGranted = false;
//            throw std::runtime_error("Unexpected API response.");
        }
        close();
        baseUi->show();
        baseUi->token = token;
        std::cout << token << std::endl;

    }

}


Login::~Login()
{
    delete uiLogin;
}
