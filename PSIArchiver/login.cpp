#include "login.h"

#include <QDesktopWidget>
#include <QScreen>
#include <QStyle>
#include <QFileInfo>
#include <stdlib.h>

#include "downloadmanager.h"
#include "messagehandlergui.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    uiLogin(new Ui::Login)
{
    uiLogin->setupUi(this);

    uiLogin->labelErrorMessage->setVisible(false);

    // connect to login UI
    connect(uiLogin->pushButtonLogin, SIGNAL (clicked()), this, SLOT (getLoginData()));

    #ifndef WINDOWS_SPECIFIC_TOOLS
    // Linux and MacOS
    urlListIngestor = QStringList{"https://gitlab.psi.ch/scicat/tools/raw/master/linux/datasetIngestorVersion.txt", "https://gitlab.psi.ch/scicat/tools/raw/master/linux/datasetIngestor"};
    urlListRetriever = QStringList{"https://gitlab.psi.ch/scicat/tools/raw/master/linux/datasetRetrieverVersion.txt", "https://gitlab.psi.ch/scicat/tools/raw/master/linux/datasetRetriever"};
    datasetIngestor = "datasetIngestor";
    datasetRetriever = "datasetRetriever";
    #else
    // Windows
    urlListIngestor = QStringList{"https://gitlab.psi.ch/scicat/tools/raw/master/windows/datasetIngestorVersion.txt", "https://gitlab.psi.ch/scicat/tools/raw/master/windows/datasetIngestor.exe"};
    urlListRetriever = QStringList{"https://gitlab.psi.ch/scicat/tools/raw/master/windows/datasetRetrieverVersion.txt", "https://gitlab.psi.ch/scicat/tools/raw/master/windows/datasetRetriever.exe"};
    datasetIngestor = "datasetIngestor.exe";
    datasetRetriever = "datasetRetriever.exe";
    #endif

}


void Login::connectToGui(Dialog *ui){
    baseUi = ui;
}

void Login::replyFinished(QNetworkReply *reply){
    qDebug() << "replyFinished";
    qDebug() << reply->readAll();

}

void Login::getLoginData(){
    QSettings settingsContainer("PSI", "SciCat");

    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();

    auto status = baseUi->scicat->authenticateUser(uname, pword);

    if (status==SciCat::authenticationStateAccount::Invalid){
        errorMessage("Please check your username and password");
        uiLogin->lineEditPassword->clear();
        return;
    } else if (status==SciCat::authenticationStateAccount::ConnectionError){
        errorMessage("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.");
        return;
    } else if (status==SciCat::authenticationStateAccount::ValidBLAccount){
        // can be removed once the pgroup selection is fixed:
        errorMessage("Beamline accounts are currently not supported.");
        uiLogin->lineEditPassword->clear();
        return;
    }

    baseUi->setAvailPgroups();

    baseUi->ui->labelUsername->setText("Username: " + baseUi->scicat->user()->getUsername());
    baseUi->ui->labelServer->setText("Server: " + baseUi->scicat->getServerName());

    // check version and update CLI tools
    updateCLITools();

#ifndef WINDOWS_SPECIFIC_TOOLS
    // get Kerberos token
    if (!baseUi->scicat->user()->getIsBeamlineAccount()){
        authenticateUser(pword);
    } else {
        openGate();
    }
#else
    openGate();
#endif

}

void Login::authenticateUser(QString pass){
    kerberosAuth = new QProcess(this);

    kerberosAuth->setProcessChannelMode(QProcess::MergedChannels);
    connect(kerberosAuth, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(authenticationFailed(QProcess::ProcessError)));
    connect(kerberosAuth, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(authenticationFinished(int)));
    connect(kerberosAuth, SIGNAL(readyReadStandardOutput()), this, SLOT(readKeberosOutput()));

    QStringList argument(baseUi->scicat->user()->getUsername());
    kerberosAuth->start("kinit", argument);
    const QByteArray byteArray = pass.toUtf8();
    const char *passConv = byteArray.constData();
    kerberosAuth->write(passConv,pass.length()+1);
    kerberosAuth->closeWriteChannel();


}

void Login::readKeberosOutput(){

    qDebugLine("Login") << "reading output";
    while(kerberosAuth->canReadLine()){
        auto tmp = kerberosAuth->readLine();
        qDebugLine("Login") << tmp;
    }

}

void Login::authenticationFinished(int val){
    qDebugLine("Login") << "Authentication finished with exit code " << val;
#ifndef SKIP_KERBEROS_AUTH
    if (val!=0){
        errorMessage("Kerberos authentication failed.");
        return;
    }
#endif
    openGate();
}

void Login::errorMessage(QString msg){
    uiLogin->labelErrorMessage->setText(msg);
    uiLogin->labelErrorMessage->setVisible(true);
    QTimer::singleShot(5000, uiLogin->labelErrorMessage, &QLabel::hide);
}

void Login::updateCLITools(){
    QSettings settingsContainer("PSI", "SciCat");
//    settingsContainer.value("datsetIngestorBinary").toString()

    bool filesOnPath = (fileExistsOnPath("datasetIngestor") || fileExistsOnPath("datasetRetriever"));
    if (!filesOnPath){
        // executables on not on $PATH. Let's download everything
        QString dirpath = "./bin";
        if (!QDir(dirpath).exists()){
            if (!QDir().mkpath(dirpath)){
                MessageHandlerGUI::sendWarning(uiLogin->labelErrorMessage, "Could not download dependencies.", 5000);
            }
            MessageHandlerGUI::sendMessage(uiLogin->labelErrorMessage, "Downloading dependencies...", 5000);
            downloadDatasetIngestor();
            downloadDatasetRetriever();
        } else {
            if (!fileIsUpToDate("datasetIngestorVersion.txt") || !QFile(QDir("./bin").filePath(datasetIngestor)).exists()){
                MessageHandlerGUI::sendMessage(uiLogin->labelErrorMessage, "Downloading dependencies...", 5000);
                downloadDatasetIngestor();
            }
            if (!fileIsUpToDate("datasetRetrieverVersion.txt") || !QFile(QDir("./bin").filePath(datasetRetriever)).exists()){
                MessageHandlerGUI::sendMessage(uiLogin->labelErrorMessage, "Downloading dependencies...", 5000);
                downloadDatasetRetriever();
            }
        }
        settingsContainer.setValue("datasetIngestorBinary", "./bin/" + datasetIngestor);
        settingsContainer.setValue("datasetRetrieverBinary", "./bin/" + datasetRetriever);
    } else {
        settingsContainer.setValue("datasetIngestorBinary", datasetIngestor);
        settingsContainer.setValue("datasetRetrieverBinary", datasetRetriever);
    }

}

void Login::downloadDatasetIngestor(){
    DownloadManager* ingestor = new DownloadManager;
    ingestor->setOutputPath("./bin");
    ingestor->append(urlListIngestor);
    ingestor->wait();
    delete ingestor;
}

void Login::downloadDatasetRetriever(){
    DownloadManager* retriever = new DownloadManager;
    retriever->setOutputPath("./bin");
    retriever->append(urlListRetriever);
    retriever->wait();
    delete retriever;
}

void Login::authenticationFailed(QProcess::ProcessError error){
    qDebugLine("Login") << "Authentication failed: " << error;
#ifdef SKIP_KERBEROS_AUTH
    openGate();
#else
    errorMessage("Kerberos authentication failed.");
#endif

}

void Login::openGate(){
    close();
    baseUi->show();
    auto screen = QGuiApplication::primaryScreen();
    auto rec = screen->geometry();
    int refSize;
    rec.height() > rec.width() ? refSize=rec.width()*0.8 : refSize=rec.height()*0.8;
    baseUi->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(refSize, refSize*0.89), rec));
}

bool Login::fileExistsOnPath(const QString& filename){
    QString envPath = QString(getenv("PATH"));
    for (auto p : envPath.split(":")){
        QString fPath = QDir(p).filePath(filename);
        if (QFile(fPath).exists()){
            return true;
        }
    }
    return false;
}

bool Login::fileIsUpToDate(const QString &filename){
    QString fPath = QDir("./bin").filePath(filename);
    if (!QFile(fPath).exists()){
        return false;
    } else {
        QString remoteVersion = getCurrentVersionRemote(filename).split("\n")[0];
        QFile f(fPath);
        if (!f.open(QFile::ReadOnly | QFile::Text)){
            return false;
        }
        QTextStream in(&f);
        QString currentVersion = in.readAll().split("\n")[0];
        if (currentVersion.compare(remoteVersion)==0){
            qDebugLine("Login") << "Executable is up to date (" + filename + ")";
            return true;
        }
    }
    return false;
}


QString Login::getCurrentVersionRemote(const QString &filename){
    QNetworkRequest getCurrentVersion;
    getCurrentVersion.setRawHeader("Accept", "application/json");
    QUrl serverURL = QUrl::fromEncoded( "https://gitlab.psi.ch/scicat/tools/raw/master/linux/" + filename.toUtf8());
    getCurrentVersion.setUrl(serverURL);
    QString currentVersion = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, getCurrentVersion);
    return currentVersion;
}

Login::~Login()
{
    delete uiLogin;
}
