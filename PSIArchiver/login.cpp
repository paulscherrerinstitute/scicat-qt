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
    baseUi->metadataContainer.pass = pword;
    std::string token;

    QNetworkRequest request;
    QByteArray postData=("{\"username\":\"" + uiLogin->lineEditUsername->text() + "\",\"password\":\"" + uiLogin->lineEditPassword->text() + "\"}").toUtf8();
    request.setRawHeader("Content-Type", "application/json");
    QUrl serverURL(baseUi->defaultSettings.serverListAuth[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()]);
    request.setUrl(serverURL);
    QString reply = NetworkManager::startNetworkManager(NetworkManager::PostOperation, request, postData);

    auto tmp = reply.toStdString();
    qDebugLine("Login") << tmp;
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        // try BL accounts
        QUrl serverURLBL(baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()] + "/Users/login");
        request.setUrl(serverURLBL);
        QString reply = NetworkManager::startNetworkManager(NetworkManager::PostOperation, request, postData);

        tmp = reply.toStdString();
        if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
            errorMessage("Please check your username and password");
            uiLogin->lineEditPassword->clear();
            return;
        } else {
            baseUi->metadataContainer.isBeamlineAccount = true;

            // can be removed once the pgroup selection is fixed:
            errorMessage("Beamline accounts are currently not supported.");
            uiLogin->lineEditPassword->clear();
            return;
        }
    }

    accessGranted = true;
    if (tmp.find("access_token") != std::string::npos){
        token = QJsonDocument::fromJson(reply.toUtf8()).object().find("access_token").value().toString().toStdString();
    } else if (tmp.find("id") != std::string::npos) {
        token = QJsonDocument::fromJson(reply.toUtf8()).object().find("id").value().toString().toStdString();
    } else {
        errorMessage("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.");
        accessGranted = false;
    }
    if (accessGranted){
        QNetworkRequest requestUser;
        requestUser.setRawHeader("Accept", "application/json");
        QUrl serverURL(baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()]+"Users/userInfos?access_token=" + QString::fromStdString(token));
        requestUser.setUrl(serverURL);
        reply = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestUser);
        qDebugLine("accessGranted") << reply;

        QJsonDocument userInfo = QJsonDocument::fromJson(reply.toUtf8());
        qDebugLine("Login") << userInfo;
        baseUi->metadataContainer.userMail = userInfo.object().value("currentUserEmail").toString();
        baseUi->metadataContainer.username = userInfo.object().value("currentUser").toString();
        baseUi->metadataContainer.userInfo = userInfo;
        baseUi->ui->labelUsername->setText("Username: " + baseUi->metadataContainer.username);
        baseUi->ui->labelServer->setText("Server: " + baseUi->defaultSettings.serverListNames[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()]);

        baseUi->token = token;
        baseUi->getAvailPgroups(userInfo);

        if (!baseUi->metadataContainer.isBeamlineAccount){
            authenticateUser(pword);
        } else {
            openGate();
        }

        qDebugLine("Login") << token;
    }
}

void Login::authenticateUser(QString pass){
    kerberosAuth = new QProcess(this);

    kerberosAuth->setProcessChannelMode(QProcess::MergedChannels);
    connect(kerberosAuth, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(authenticationFailed(QProcess::ProcessError)));
    connect(kerberosAuth, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(authenticationFinished(int)));
    connect(kerberosAuth, SIGNAL(readyReadStandardOutput()), this, SLOT(readKeberosOutput()));

    QStringList argument(baseUi->metadataContainer.username);
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
