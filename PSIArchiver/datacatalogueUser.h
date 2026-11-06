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

    QString getPword() const;

private:
    User(DataCatalogue * const parent);
    friend class SciCat::DataCatalogue;
    authenticationStateAccount authenticateUser(const QString& token);
    authenticationState check_token(const QString& token);
    int sendUserInfoRequest();

    void setPgroups();

    QString m_username;
    QString m_pword;
    QString m_token;
    QStringList m_pgroups;
    QJsonDocument m_userInfo;
    QString m_userMail;
    DataCatalogue * const scicatBase;
};

}
}

#endif // DATACATALOGUEUSER_H
