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
    authenticationStateAccount authenticateUser(const QString &token);
    static QStringList serverList();
    static QStringList serverListRetrieve();
    static QStringList serverListFrontend();
    static QStringList cliEnvironmentArguments(const int& serverID);
    QNetworkRequest apiRequest(const QString& endpoint, const QJsonObject& filter = QJsonObject(), const QString& filterKey = "filter") const;
    DataCatalogueCore::User* user();
    DataCatalogueCore::Experiment* experiment();
    DatasetIngestor *ingestion();
    QString getServerName() const;
    QString getFrontendURL() const;
    QString getTokenURL() const;
    int getServerID() const;
    void setServerID(const int& id);
    Dialog * baseUI;
    int requestRetrieve(const QJsonArray& datasetList);
    void startDatasetRetriever();

private:
    DataCatalogueCore::User* scicatUser;
    DataCatalogueCore::Experiment* scicatExperiment;
    DatasetIngestor* scicatIngestion;
    int serverID = 0;
    SciCat::serverType serverType = SciCat::serverType::Production;

};


}


#endif // DATACATALOGUE_H
