#include "datacatalogueUser.h"
#include "networkmanager.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

using namespace SciCat::DataCatalogueCore;



void User::updateUserInfo()
{
    sendUserInfoRequest();
    setUserMail(m_userInfo.object().value("currentUserEmail").toString());
    setUsername(m_userInfo.object().value("currentUser").toString());
    if (!m_isBeamlineAccount){
        setPgroups();
    }
}

User::User(DataCatalogue * const parent): scicatBase(parent)
{

}

SciCat::authenticationStateAccount User::authenticateUser(const QString& username, const QString& pword, const int& server)
{
    // try normal user account first
    auto state = sendAuthenticationRequest(username, pword, DataCatalogue::serverListAuth()[server]);
    if (state==authenticationState::ConnectionError){
        return authenticationStateAccount::ConnectionError;
    }
    if (state==authenticationState::Valid){
        updateUserInfo();
        return authenticationStateAccount::ValidUser;
    } else if (sendAuthenticationRequest(username, pword, DataCatalogue::serverList()[server]+ "/Users/login")==authenticationState::Valid) {
        m_isBeamlineAccount = true;
        updateUserInfo();
        return authenticationStateAccount::ValidBLAccount;
    } else {
        return authenticationStateAccount::Invalid;
    }

}


SciCat::authenticationState User::sendAuthenticationRequest(const QString& username, const QString& pword, const QString& server){
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
        if (m_token.length()==0){
            return authenticationState::ConnectionError;
        } else {
            return authenticationState::Valid;
        }

    }
}

void User::extractToken(const QString& reply){
    if (reply.contains("access_token")){
        m_token = QJsonDocument::fromJson(reply.toUtf8()).object().find("access_token").value().toString();
    } else if (reply.contains("id")){
        m_token = QJsonDocument::fromJson(reply.toUtf8()).object().find("access_token").value().toString();
    }
    qDebugLine("Token") << m_token;
}

QString User::getUserMail() const
{
    return m_userMail;
}

void User::setUserMail(const QString &userMail)
{
    m_userMail = userMail;
}

QStringList User::getPgroups() const
{
    return m_pgroups;
}

void User::setPgroups()
{
    qDebugLine("UpdatePgroups:") << m_userInfo;
    for (auto v:m_userInfo.object().find("currentGroups").value().toArray()){
        m_pgroups.push_back(v.toString());
    }
}

bool User::getIsBeamlineAccount() const
{
    return m_isBeamlineAccount;
}

QString User::getUsername() const
{
    return m_username;
}

void User::setUsername(const QString &value)
{
    m_username = value;
}

QString User::getToken() const
{
    return m_token;
}

void User::setToken(const QString &value)
{
    m_token = value;
}

void User::sendUserInfoRequest(){
    QNetworkRequest requestUser;
    requestUser.setRawHeader("Accept", "application/json");
    QUrl serverURL(DataCatalogue::serverList()[scicatBase->serverID]+"Users/userInfos?access_token=" + m_token);
    requestUser.setUrl(serverURL);
    qDebugLine("sendUserInfoRequest") << serverURL;
    auto nwm = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestUser);
    auto reply = nwm->pull();

    qDebugLine("sendUserInfoRequest") << reply;
    m_userInfo = QJsonDocument::fromJson(reply.toUtf8());

}
