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

void Login::getLoginData(){
    QSettings settingsContainer("PSI", "SciCat");

    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();
    baseUi->metadataContainer.pass = pword;
    std::string token;
    auto tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverListAuth[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "'");
    qDebug() << tmp;
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        // try BL accounts
        tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "Users/login'");
        if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
//            QMessageBox::information(this, tr("Authentication error"), tr("Please check your username and password") );
            errorMessage("Please check your username and password");
            uiLogin->lineEditPassword->clear();
            return;
        } else {
            baseUi->metadataContainer.isBeamlineAccount = true;
        }
    }

    accessGranted = true;
    if (tmp.find("access_token") != std::string::npos){
        token = utils::extractFromJson(tmp, "access_token");
    } else if (tmp.find("id") != std::string::npos) {
        token = utils::extractFromJson(tmp, "id");
    } else {
//        QMessageBox::information(this, tr("Connection error"), tr("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.") );
        errorMessage("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.");
        accessGranted = false;
    }
    if (accessGranted){
        auto tmpUserInfo = utils::system_call("curl -s -X GET --header 'Accept: application/json' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "/Users/userInfos?access_token=" + token + "'");
        baseUi->metadataContainer.userMail = QString::fromStdString(utils::extractFromJson(tmpUserInfo, "currentUserEmail"));
        baseUi->metadataContainer.username = QString::fromStdString(utils::extractFromJson(tmpUserInfo, "currentUser"));
        baseUi->metadataContainer.userInfo = tmpUserInfo;
        baseUi->ui->labelUsername->setText("Username: " + baseUi->metadataContainer.username);
        baseUi->ui->labelServer->setText("Server: " + baseUi->defaultSettings.serverListNames[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()]);
        //            qDebug() << "curl -s -X GET --header 'Accept: application/json' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "Users/userInfos?access_token=" + token + "'";
        //            qDebug() << baseUi->metadataContainer.userMail;
        //                    qDebug() << tmpUserInfo;
        baseUi->token = token;
        baseUi->getAvailPgroups();

        if (!baseUi->metadataContainer.isBeamlineAccount){
            authenticateUser(pword);
        } else {
            openGate();
        }

        qDebug() << token;
    }
}

void Login::authenticateUser(QString pass){
    kerberosAuth = new QProcess(this);

    kerberosAuth->setProcessChannelMode(QProcess::MergedChannels);
    connect(kerberosAuth, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(authenticationFailed(QProcess::ProcessError)));
    connect(kerberosAuth, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(authenticationFinished(int)));

    QStringList argument(baseUi->metadataContainer.username);
    qDebug() << (const char*)pass.data();
    qDebug() << argument;
    kerberosAuth->start("kinit ", argument);
    kerberosAuth->write((const char*)pass.data(),pass.length());
    kerberosAuth->closeWriteChannel();
    kerberosAuth->waitForFinished(5000);
//    qDebug() << kerberosAuth->error
    qDebug() << kerberosAuth->readAllStandardError();


}

void Login::authenticationFinished(int val){
    qDebug() << "Authentication finished with exit code " << val;
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
    qDebug() << "Authentication failed: " << error;
    errorMessage("Kerberos authentication failed.");
}

void Login::openGate(){
    close();
    baseUi->show();
}

Login::~Login()
{
    delete uiLogin;
}
