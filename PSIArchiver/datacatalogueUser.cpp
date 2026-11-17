#include "datacatalogueUser.h"
#include "networkmanager.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

using namespace SciCat::DataCatalogueCore;



void User::updateUserInfo()
{
    // the identity profile holds the institutional email, as used by scicat-cli; the
    // email in the user record can be a placeholder from the login provider
    const QString identityMail = requestIdentityMail();
    setUserMail(identityMail.isEmpty() ? m_userInfo.object().value("email").toString() : identityMail);
    setUsername(m_userInfo.object().value("username").toString());
    setPgroups();
}

User::User(DataCatalogue * const parent): scicatBase(parent)
{

}

SciCat::authenticationStateAccount User::authenticateUser(const QString& token)
{
    const auto state = check_token(token);
    if (state==authenticationState::Valid){
        updateUserInfo();
        return authenticationStateAccount::ValidUser;
    }
    if (state==authenticationState::ConnectionError){
        return authenticationStateAccount::ConnectionError;
    }
    return authenticationStateAccount::Invalid;
}

SciCat::authenticationState User::check_token(const QString& token){
    if (token.isEmpty()){
        return authenticationState::Invalid;
    }
    m_token = token;
    const int httpStatus = sendUserInfoRequest();
    qDebugLine("check_token") << httpStatus << m_userInfo;
    if (httpStatus==0 || (httpStatus>=200 && httpStatus<300 && !m_userInfo.isObject())){
        m_token.clear();
        return authenticationState::ConnectionError;
    }
    if (httpStatus<200 || httpStatus>=300 || m_userInfo.object().value("username").toString().isEmpty()){
        m_token.clear();
        return authenticationState::Invalid;
    }
    return authenticationState::Valid;
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
    m_pgroups.clear();
    for (auto v:m_userInfo.object().find("currentGroups").value().toArray()){
        m_pgroups.push_back(v.toString());
    }
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

QString User::requestIdentityMail() const
{
    const QString userId = m_userInfo.object().value("_id").toString();
    if (userId.isEmpty()){
        return QString();
    }
    const QString endpoint = "users/" + QString::fromUtf8(QUrl::toPercentEncoding(userId)) + "/userIdentity";
    const QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, scicatBase->apiRequest(endpoint, ApiVersion::V3));
    const QJsonArray emails = QJsonDocument::fromJson(reply.toUtf8()).object().value("profile").toObject().value("emails").toArray();
    return emails.isEmpty() ? QString() : emails.first().toObject().value("value").toString();
}

int User::sendUserInfoRequest(){
    QNetworkRequest requestUser = scicatBase->apiRequest("auth/whoami", ApiVersion::V3);
    auto nwm = NetworkManager::startNetworkManager(NetworkManager::GetOperation, requestUser);
    auto reply = nwm->pull();

    qDebugLine("sendUserInfoRequest") << reply;
    m_userInfo = QJsonDocument::fromJson(reply.toUtf8());
    return nwm->getReply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}
