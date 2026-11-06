#include "datacatalogue.h"


using namespace SciCat;

DataCatalogue::DataCatalogue(Dialog *parent, const int& scicatServerID): serverID(scicatServerID), baseUI(parent), serverType(static_cast<SciCat::serverType>(serverID))
{
    scicatUser = new DataCatalogueCore::User(this);
    scicatExperiment = new DataCatalogueCore::Experiment(this);
    scicatIngestion = new DatasetIngestor(parent);
}

DataCatalogue::~DataCatalogue()
{
    delete scicatUser;
    delete scicatExperiment;
}



authenticationStateAccount DataCatalogue::authenticateUser(const QString& token){
    return scicatUser->authenticateUser(token);
}

QStringList DataCatalogue::serverList(){
    return QStringList{"https://dacat.psi.ch/api/v3/", "https://dacat-qa.psi.ch/api/v3/", "https://scicat.development.psi.ch/api/v3/"};
}

QStringList DataCatalogue::serverListRetrieve(){
    return QStringList{"pb-retrieve.psi.ch", "pbt-retrieve.psi.ch", "arematest2in.psi.ch"};
}

// environment flags of the scicat-cli matching the entries of serverList()
QStringList DataCatalogue::cliEnvironmentArguments(const int& serverID){
    switch (static_cast<SciCat::serverType>(serverID)){
    case SciCat::serverType::QA: return QStringList{"--testenv"};
    case SciCat::serverType::Development: return QStringList{"--devenv"};
    default: return QStringList();
    }
}

QNetworkRequest DataCatalogue::apiRequest(const QString& endpoint, const QJsonObject& filter, const QString& filterKey) const{
    QByteArray url = DataCatalogue::serverList()[serverID].toUtf8() + endpoint.toUtf8();
    if (!filter.isEmpty()){
        url += "?" + filterKey.toUtf8() + "=" + QUrl::toPercentEncoding(QJsonDocument(filter).toJson(QJsonDocument::Compact));
    }
    QNetworkRequest request(QUrl::fromEncoded(url));
    request.setRawHeader("Accept", "application/json");
    if (!scicatUser->getToken().isEmpty()){
        request.setRawHeader("Authorization", "Bearer " + scicatUser->getToken().toUtf8());
    }
    return request;
}

DataCatalogueCore::User* DataCatalogue::user()
{
    return scicatUser;
}
DataCatalogueCore::Experiment* DataCatalogue::experiment()
{
    return scicatExperiment;
}

DatasetIngestor* DataCatalogue::ingestion()
{
    return scicatIngestion;
}

QString DataCatalogue::getServerName() const
{
    switch (serverType) {
    case SciCat::serverType::Production: return "Production";
    case SciCat::serverType::QA: return "QA";
    case SciCat::serverType::Development: return "Development";
    default: return "";
    }
}

int DataCatalogue::requestRetrieve(const QJsonArray& datasetList){
    QSettings settingsContainer("PSI", "SciCat");

    QJsonObject request;
    request["emailJobInitiator"] = scicatUser->getUserMail();
    request["type"] = "retrieve";
    {
        QJsonObject jobParams;
        jobParams["username"] = scicatUser->getUsername();
        jobParams["destinationPath"] = "/archive/retrieve";
        jobParams["option"] = "PSI";
        request["jobParams"] = jobParams;
    }
    request["datasetList"] = datasetList;

    if (datasetList.size()>0){
        QNetworkRequest networkRequest = apiRequest("jobs");
        networkRequest.setRawHeader("Content-Type", "application/json");
        QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::PostOperation, networkRequest, QString(QJsonDocument(request).toJson(QJsonDocument::Compact)).toUtf8());
    }
    return 0;
}

