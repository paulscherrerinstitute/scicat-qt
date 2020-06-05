#include "login.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    uiLogin(new Ui::Login)
{
    uiLogin->setupUi(this);

    uiLogin->labelErrorMessage->setVisible(false);

    // connect to login UI
    connect(uiLogin->pushButtonLogin, SIGNAL (clicked()), this, SLOT (getLoginData()));
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
        baseUi->metadataContainer.isBeamlineAccount = true;
        // can be removed once the pgroup selection is fixed:
        errorMessage("Beamline accounts are currently not supported.");
        uiLogin->lineEditPassword->clear();
        return;
    } else {
        baseUi->metadataContainer.isBeamlineAccount = false;
    }

    baseUi->setAvailPgroups();
    baseUi->metadataContainer.pass = pword;

    baseUi->ui->labelUsername->setText("Username: " + baseUi->scicat->user()->getUsername());
    baseUi->ui->labelServer->setText("Server: " + baseUi->scicat->getServerName());

    if (!baseUi->metadataContainer.isBeamlineAccount){
        authenticateUser(pword);
    } else {
        openGate();
    }

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
}

Login::~Login()
{
    delete uiLogin;
}
