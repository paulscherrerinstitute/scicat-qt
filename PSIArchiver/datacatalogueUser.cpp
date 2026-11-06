#include "datacatalogueUser.h"
#include "networkmanager.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

using namespace SciCat::DataCatalogueCore;



void User::updateUserInfo()
{
    sendUserInfoRequest();
    setUserMail(m_userInfo.object().value("email").toString());
    setUsername(m_userInfo.object().value("username").toString());
    if (!m_isBeamlineAccount){
        setPgroups();
    }
}

User::User(DataCatalogue * const parent): scicatBase(parent)
{

}

SciCat::authenticationStateAccount User::authenticateUser(const QString& username, const QString& pword, const int& server)
{
#ifdef WINDOWS_SPECIFIC_TOOLS
    m_pword = pword;
#endif
    auto state = this->check_token(username, pword);
    if (state==authenticationState::Valid){
        updateUserInfo();
        return authenticationStateAccount::ValidUser;
    } else {
        // try normal user account first
        state = sendAuthenticationRequest(username, pword, DataCatalogue::apiUrl(server, ApiVersion::V3, "auth/ldap"));
        if (state==authenticationState::ConnectionError){
            return authenticationStateAccount::ConnectionError;
        }
        if (state==authenticationState::Valid){
            updateUserInfo();
            return authenticationStateAccount::ValidUser;
        } else if (sendAuthenticationRequest(username, pword, DataCatalogue::apiUrl(server, ApiVersion::V3, "auth/login"))==authenticationState::Valid) {
            m_isBeamlineAccount = true;
            updateUserInfo();
            return authenticationStateAccount::ValidBLAccount;
        } else {
            return authenticationStateAccount::Invalid;
        }
    }


}

SciCat::authenticationState User::check_token(const QString& username, const QString& token){
    this->m_token = token;
    this->sendUserInfoRequest();
    qDebugLine("check_token: ") << this->m_userInfo;
    auto current_user = this->m_userInfo.object().value("username").toString();
    qDebugLine("check_token: ") << current_user;
    if (username == current_user){
        return authenticationState::Valid;
    } else {
        return authenticationState::Invalid;
    }

}


SciCat::authenticationState User::sendAuthenticationRequest(const QString& username, const QString& pword, const QString& server){
    QNetworkRequest request;
    QJsonObject credentials;
    credentials["username"] = username;
    credentials["password"] = pword;
    request.setRawHeader("Content-Type", "application/json");
    QUrl serverURL(server);
    request.setUrl(serverURL);
    auto nwm = NetworkManager::startNetworkManager(NetworkManager::PostOperation, request, QJsonDocument(credentials).toJson(QJsonDocument::Compact));
    QString reply = nwm->pull();
    int httpStatus = nwm->getReply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebugLine("sendAuthenticationRequest") << httpStatus;
    if (httpStatus==0){
        // no HTTP response at all
        return authenticationState::ConnectionError;
    } else if (httpStatus<200 || httpStatus>=300){
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
    m_token = QJsonDocument::fromJson(reply.toUtf8()).object().value("access_token").toString();
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

QString User::getPword() const
{
    return m_pword;
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
    QNetworkRequest requestUser = scicatBase->apiRequest("auth/whoami", ApiVersion::V3);
    auto nwm = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestUser);
    auto reply = nwm->pull();

    qDebugLine("sendUserInfoRequest") << reply;
    m_userInfo = QJsonDocument::fromJson(reply.toUtf8());

}
