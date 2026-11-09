#ifndef DATACATALOGUE_H
#define DATACATALOGUE_H
#include "dialog.h"
#include "datacatalogueClasses.h"
#include "datacatalogueExperiment.h"
#include "datacatalogueUser.h"
#include "datasetretriever.h"

namespace SciCat {

// all items of a list endpoint, fetched page by page; on Failed, `items` holds the pages loaded before the error
struct PagedResult
{
    enum class Status {Complete, Capped, Failed};
    QJsonArray items;
    Status status = Status::Complete;
};

class DataCatalogue
{
public:
    // the backend returns at most 100 items per list request unless a limit is given
    static constexpr int listPageSize = 500;
    static constexpr int listMaxPages = 100;
    DataCatalogue(Dialog* parent, const int& serverID);
    ~DataCatalogue();
    authenticationStateAccount authenticateUser(const QString &username, const QString &pword);
    static QStringList serverList();
    static QStringList serverListRetrieve();
    static QStringList cliEnvironmentArguments(const int& serverID);
    static QString apiUrl(const int& serverID, ApiVersion version, const QString& endpoint);
    QNetworkRequest apiRequest(const QString& endpoint, ApiVersion version, const QJsonObject& filter = QJsonObject(), const QString& filterKey = "filter") const;
    PagedResult fetchAllPages(const QString& endpoint, ApiVersion version, QJsonObject filter, QJsonObject limits) const;
    DataCatalogueCore::User* user();
    DataCatalogueCore::Experiment* experiment();
    DatasetIngestor *ingestion();
    QString getServerName() const;
    int serverID;
    Dialog * baseUI;
    bool requestRetrieve(const QJsonArray& datasetList, QString& errorMessage);
    void startDatasetRetriever();

private:
    DataCatalogueCore::User* scicatUser;
    DataCatalogueCore::Experiment* scicatExperiment;
    DatasetIngestor* scicatIngestion;
    SciCat::serverType serverType;

};


}


#endif // DATACATALOGUE_H
