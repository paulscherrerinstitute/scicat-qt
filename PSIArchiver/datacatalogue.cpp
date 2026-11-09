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
    return QStringList{"https://dacat.psi.ch/api/", "https://dacat-qa.psi.ch/api/", "https://scicat.development.psi.ch/api/"};
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

QString DataCatalogue::apiUrl(const int& serverID, ApiVersion version, const QString& endpoint){
    const QString prefix = (version==ApiVersion::V4) ? "v4/" : "v3/";
    return DataCatalogue::serverList()[serverID] + prefix + endpoint;
}

QNetworkRequest DataCatalogue::apiRequest(const QString& endpoint, ApiVersion version, const QJsonObject& filter, const QString& filterKey) const{
    QByteArray url = apiUrl(serverID, version, endpoint).toUtf8();
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

PagedResult DataCatalogue::fetchAllPages(const QString& endpoint, ApiVersion version, QJsonObject filter, QJsonObject limits) const{
    PagedResult result;
    for (int page=0; page<listMaxPages; ++page){
        limits["limit"] = listPageSize;
        limits["skip"] = page*listPageSize;
        filter["limits"] = limits;
        auto manager = NetworkManager::startNetworkManager(NetworkManager::GetOperation, apiRequest(endpoint, version, filter));
        const QString body = manager->pull();
        const QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
        const int httpStatus = manager->getReply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus<200 || httpStatus>=300 || !doc.isArray()){
            qWarning().noquote() << "[DataCatalogue]" << endpoint << "request failed (HTTP" << httpStatus << "):" << body;
            result.status = PagedResult::Status::Failed;
            return result;
        }
        const QJsonArray items = doc.array();
        for (const auto& item : items){
            result.items.append(item);
        }
        if (items.size()<listPageSize){
            return result;
        }
    }
    qWarning() << "[DataCatalogue]" << endpoint << "stopped after" << listMaxPages << "pages";
    result.status = PagedResult::Status::Capped;
    return result;
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

bool DataCatalogue::requestRetrieve(const QJsonArray& datasetList, QString& errorMessage){
    if (datasetList.isEmpty()){
        errorMessage = "None of the selected datasets can be retrieved.";
        return false;
    }

    QJsonObject jobParams;
    jobParams["username"] = scicatUser->getUsername();
    jobParams["destinationPath"] = "/archive/retrieve";
    jobParams["option"] = "PSI";
    jobParams["datasetList"] = datasetList;

    QJsonObject request;
    request["type"] = "retrieve";
    request["jobParams"] = jobParams;
    request["ownerUser"] = scicatUser->getUsername();
    request["ownerGroup"] = scicatExperiment->getOwnerGroup();
    if (!scicatUser->getUserMail().isEmpty()){
        request["contactEmail"] = scicatUser->getUserMail();
    }

    QNetworkRequest networkRequest = apiRequest("jobs", ApiVersion::V4);
    networkRequest.setRawHeader("Content-Type", "application/json");
    auto manager = NetworkManager::startNetworkManager(NetworkManager::PostOperation, networkRequest, QJsonDocument(request).toJson(QJsonDocument::Compact));
    const QString replyBody = manager->pull();
    const QNetworkReply *reply = manager->getReply();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (httpStatus==0){
        qWarning().noquote() << "[DataCatalogue] retrieve job request failed:" << reply->errorString();
        errorMessage = QString("Could not connect to SciCat: %1").arg(reply->errorString());
        return false;
    }
    if (reply->error()!=QNetworkReply::NoError || httpStatus<200 || httpStatus>=300){
        qWarning().noquote() << "[DataCatalogue] retrieve job request failed (HTTP" << httpStatus << "):" << replyBody;
        // SciCat returns {"message": ...}, for validation errors a list of messages
        const QJsonObject body = QJsonDocument::fromJson(replyBody.toUtf8()).object();
        QString serverMessage = body.value("error").toObject().value("message").toString();
        if (serverMessage.isEmpty()){
            const QJsonValue message = body.value("message");
            QStringList messages;
            for (const auto& m : message.isArray() ? message.toArray() : QJsonArray{message}){
                messages << m.toString();
            }
            serverMessage = messages.join("; ").trimmed();
        }
        if (serverMessage.isEmpty()){
            serverMessage = reply->errorString();
        }
        errorMessage = QString("SciCat rejected the retrieve request (HTTP %1): %2").arg(httpStatus).arg(serverMessage);
        return false;
    }
    return true;
}

