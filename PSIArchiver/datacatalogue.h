#ifndef DATACATALOGUE_H
#define DATACATALOGUE_H

#include "datacatalogueClasses.h"
#include "datacatalogueExperiment.h"
#include "datacatalogueUser.h"

namespace SciCat {

class DataCatalogue
{
public:
    DataCatalogue(const int& serverID);
    ~DataCatalogue();
    authenticationStateAccount authenticateUser(const QString &username, const QString &pword);
    static QStringList serverList();
    static QStringList serverListAuth();
    QJsonDocument getUserInfo();
    void setPgroup(const QString& pgroup);
    DataCatalogueCore::User* user();
    DataCatalogueCore::Experiment* experiment();
    void ingest();
    int serverID;

private:
    DataCatalogueCore::User* scicatUser;
    DataCatalogueCore::Experiment* scicatExperiment;
    SciCat::serverType serverType;

};


}


#endif // DATACATALOGUE_H
