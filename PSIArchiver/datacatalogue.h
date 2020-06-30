#ifndef DATACATALOGUE_H
#define DATACATALOGUE_H
#include "dialog.h"
#include "datacatalogueClasses.h"
#include "datacatalogueExperiment.h"
#include "datacatalogueUser.h"
#include "datasetretriever.h"

namespace SciCat {

class DataCatalogue
{
public:
    DataCatalogue(Dialog* parent, const int& serverID);
    ~DataCatalogue();
    authenticationStateAccount authenticateUser(const QString &username, const QString &pword);
    static QStringList serverList();
    static QStringList serverListAuth();
    static QStringList serverListRetrieve();
    DataCatalogueCore::User* user();
    DataCatalogueCore::Experiment* experiment();
    DatasetIngestor *ingestion();
    QString getServerName() const;
    int serverID;
    Dialog * baseUI;
    int requestRetrieve(const QJsonArray& datasetList);
    void startDatasetRetriever();

private:
    DataCatalogueCore::User* scicatUser;
    DataCatalogueCore::Experiment* scicatExperiment;
    DatasetIngestor* scicatIngestion;
    SciCat::serverType serverType;

};


}


#endif // DATACATALOGUE_H
