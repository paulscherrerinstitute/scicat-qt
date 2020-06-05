#include "datacatalogueUser.h"
#include "networkmanager.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

using namespace SciCat;

DataCatalogueCore::User* DataCatalogue::user()
{
    return scicatUser;
}

QJsonDocument DataCatalogueCore::User::getUserInfo()
{
    sendUserInfoRequest();
    return userInfo;
}

DataCatalogueCore::User::User(DataCatalogue * const parent): scicatBase(parent)
{

}

authenticationStateAccount DataCatalogueCore::User::authenticateUser(const QString& username, const QString& pword, const int& server)
{
    // try normal user account first
    if (sendAuthenticationRequest(username, pword, DataCatalogue::serverListAuth()[server])==authenticationState::Valid){
        return authenticationStateAccount::ValidUser;
    } else if (sendAuthenticationRequest(username, pword, DataCatalogue::serverList()[server]+ "/Users/login")==authenticationState::Valid) {
        return authenticationStateAccount::ValidBLAccount;
    } else {
        return authenticationStateAccount::Invalid;
    }

}


authenticationState DataCatalogueCore::User::sendAuthenticationRequest(const QString& username, const QString& pword, const QString& server){
    QNetworkRequest request;
    QByteArray postData=("{\"username\":\"" + username + "\",\"password\":\"" + pword + "\"}").toUtf8();
    request.setRawHeader("Content-Type", "application/json");
    QUrl serverURL(server);
    request.setUrl(serverURL);
    QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::PostOperation, request, postData);
    qDebugLine("sendAuthenticationRequest") << reply;
    auto tmp = reply.toStdString();
    if ((tmp.compare("\"authentication error\"")==0) || (tmp.find("error") != std::string::npos)){
        return authenticationState::Invalid;
    } else {
        extractToken(reply);
        return authenticationState::Valid;
    }
}

void DataCatalogueCore::User::extractToken(const QString& reply){
    if (reply.contains("access_token")){
        token = QJsonDocument::fromJson(reply.toUtf8()).object().find("access_token").value().toString();
    } else if (reply.contains("id")){
        token = QJsonDocument::fromJson(reply.toUtf8()).object().find("access_token").value().toString();
    }
    qDebugLine("Token") << token;
}

void DataCatalogueCore::User::sendUserInfoRequest(){
    QNetworkRequest requestUser;
    requestUser.setRawHeader("Accept", "application/json");
    QUrl serverURL(DataCatalogue::serverList()[scicatBase->serverID]+"Users/userInfos?access_token=" + token);
    requestUser.setUrl(serverURL);

    auto nwm = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestUser);
    auto reply = nwm->pull();

    qDebugLine("sendUserInfoRequest") << reply;
    userInfo = QJsonDocument::fromJson(reply.toUtf8());

}
