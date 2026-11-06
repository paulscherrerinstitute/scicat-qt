#include "login.h"

#include <QScreen>
#include <QStyle>
#include <QFileInfo>
#include <QStandardPaths>
#include <stdlib.h>
#include "settings.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    uiLogin(new Ui::Login)
{
    uiLogin->setupUi(this);
    this->setWindowFlag(Qt::WindowContextHelpButtonHint,false);

    uiLogin->labelErrorMessage->setVisible(false);

    uiLogin->labelHint->setTextFormat(Qt::RichText);
    uiLogin->labelHint->setOpenExternalLinks(true);

    // connect to login UI
    connect(uiLogin->pushButtonLogin, SIGNAL (clicked()), this, SLOT (getLoginData()));
    connect(uiLogin->pushButtonSettings, SIGNAL (clicked()), this, SLOT (openSettings()));
}


void Login::connectToGui(Dialog *ui){
    baseUi = ui;
    updateServerLabel();
}

void Login::openSettings(){
    Settings sett(baseUi, Settings::Context::BeforeLogin, this);
    if (sett.exec()!=QDialog::Accepted){
        return;
    }

    // nobody is authenticated yet, so a server change can be applied right away
    QSettings settingsContainer("PSI", "SciCat");
    baseUi->scicat->setServerID(settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt());
    updateServerLabel();
}

void Login::updateServerLabel(){
    uiLogin->labelServer->setText("Server: " + baseUi->scicat->getServerName());
    const QUrl url(baseUi->scicat->getTokenURL());
    uiLogin->labelHint->setText(QString("Login with your personal PSI account and a new token from:<br><a href=\"%1\">%2</a>")
                                    .arg(url.toString().toHtmlEscaped(), (url.host() + url.path()).toHtmlEscaped()));
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
        errorMessage("Please check your username and token. To request a new token, please login with your normal user account at " + baseUi->scicat->getTokenURL() + ".");
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

    // locate the scicat-cli executable
    updateCLITools();

    openGate();

}

void Login::errorMessage(QString msg){
    uiLogin->labelErrorMessage->setText(msg);
    uiLogin->labelErrorMessage->setVisible(true);
    QTimer::singleShot(5000, uiLogin->labelErrorMessage, &QLabel::hide);
}

void Login::updateCLITools(){
    QSettings settingsContainer("PSI", "SciCat");

    // prefer the scicat-cli bundled with the GUI, then the one on $PATH
    const QDir appDir(QCoreApplication::applicationDirPath());
    QString scicatCli = QStandardPaths::findExecutable("scicat-cli", QStringList{appDir.path(), appDir.filePath("../libexec/scicat")});
    if (scicatCli.isEmpty()){
        scicatCli = QStandardPaths::findExecutable("scicat-cli");
    }
    if (scicatCli.isEmpty()){
        qDebugLine("Login") << "scicat-cli not found, keeping" << settingsContainer.value("scicatCliBinary").toString();
        return;
    }
    settingsContainer.setValue("scicatCliBinary", scicatCli);
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

Login::~Login()
{
    delete uiLogin;
}
