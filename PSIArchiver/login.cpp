#include "login.h"

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
    QSettings settingsContainer("PSI", "SciCat");

    QString uname = uiLogin->lineEditUsername->text();
    QString pword = uiLogin->lineEditPassword->text();
    std::string token;
    auto tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverListAuth[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "'");
    qDebug() << tmp;
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        // try BL accounts
        tmp = utils::system_call("curl -s -X POST --header 'Content-Type: application/json'  -d '{\"username\":\"" + uiLogin->lineEditUsername->text().toStdString() + "\",\"password\":\"" + uiLogin->lineEditPassword->text().toStdString() + "\"}' '" + baseUi->defaultSettings.serverList[settingsContainer.value("serverID", baseUi->defaultSettings.serverID).toInt()].toStdString() + "Users/login'");
        if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
            QMessageBox::information(this, tr("Authentication error"), tr("Please check your username and password") );
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
        QMessageBox::information(this, tr("Connection error"), tr("Could not connect to SciCat.\nPlease make sure that you are within the PSI intranet and try again.") );
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
        close();
        baseUi->token = token;
        baseUi->getAvailPgroups();
        baseUi->show();
        qDebug() << token;
    }
}


Login::~Login()
{
    delete uiLogin;
}
