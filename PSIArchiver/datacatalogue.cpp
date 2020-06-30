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



authenticationStateAccount DataCatalogue::authenticateUser(const QString& username, const QString& pword){
    return scicatUser->authenticateUser(username, pword, serverID);
}

QStringList DataCatalogue::serverList(){
    return QStringList{"https://dacat.psi.ch/api/v3/", "https://dacat-qa.psi.ch/api/v3/", "https://dacat-development.psi.ch/api/v3/"};
}

QStringList DataCatalogue::serverListAuth(){
    return QStringList{"https://dacat.psi.ch/auth/msad", "https://dacat-qa.psi.ch/auth/msad", "https://dacat-development.psi.ch/auth/msad"};
}

QStringList DataCatalogue::serverListRetrieve(){
    return QStringList{"pb-retrieve.psi.ch", "pbt-retrieve.psi.ch", "arematest2in.psi.ch"};
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
        request["jobParams"] = jobParams;
    }
    request["datasetList"] = datasetList;

    if (datasetList.size()>0){
        QNetworkRequest networkRequest;
        networkRequest.setRawHeader("Content-Type", "application/json");
        QUrl serverURL(DataCatalogue::serverList()[serverID].toUtf8() + "Jobs?access_token=" + scicatUser->m_token);
        networkRequest.setUrl(serverURL);
        QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::PostOperation, networkRequest, QString(QJsonDocument(request).toJson(QJsonDocument::Compact)).toUtf8());
    }
    return 0;
}

