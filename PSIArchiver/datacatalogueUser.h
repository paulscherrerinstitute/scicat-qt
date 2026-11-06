#ifndef DATACATALOGUEUSER_H
#define DATACATALOGUEUSER_H

#include "datacatalogueClasses.h"

namespace SciCat {
namespace DataCatalogueCore {

class User
{
public:
    QStringList getJobs();
    void updateUserInfo();

    QString getToken() const;
    void setToken(const QString &value);

    QString getUsername() const;
    void setUsername(const QString &value);

    QStringList getPgroups() const;

    QString getUserMail() const;
    void setUserMail(const QString &userMail);

    bool getIsBeamlineAccount() const;


private:
    User(DataCatalogue * const parent);
    friend class SciCat::DataCatalogue;
    authenticationStateAccount authenticateUser(const QString& username, const QString& pword, const int& server);
    authenticationState sendAuthenticationRequest(const QString& username, const QString& pword, const QString& server);
    authenticationState check_token(const QString& username, const QString& token);
    void sendUserInfoRequest();
    void extractToken(const QString& reply);

    void setPgroups();

    QString m_username;
    QString m_token;
    QStringList m_pgroups;
    QJsonDocument m_userInfo;
    QString m_userMail;
    DataCatalogue * const scicatBase;
    bool m_isBeamlineAccount = false;
};

}
}

#endif // DATACATALOGUEUSER_H
