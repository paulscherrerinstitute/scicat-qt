#ifndef DATACATALOGUEUSER_H
#define DATACATALOGUEUSER_H

#include "datacatalogueClasses.h"

namespace SciCat {
namespace DataCatalogueCore {

class User
{
public:
    QStringList getJobs();
    QJsonDocument getUserInfo();
private:
    User(DataCatalogue * const parent);
    friend class SciCat::DataCatalogue;
    authenticationStateAccount authenticateUser(const QString& username, const QString& pword, const int& server);
    authenticationState sendAuthenticationRequest(const QString& username, const QString& pword, const QString& server);
    void sendUserInfoRequest();
    void extractToken(const QString& reply);

    QString username;
    QString pword;
    QString token;
    QString pgroup;
    QString proposal;
    QJsonDocument userInfo;
    DataCatalogue * const scicatBase;
};

}
}

#endif // DATACATALOGUEUSER_H
