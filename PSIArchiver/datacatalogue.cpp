#include "datacatalogue.h"


using namespace SciCat;

DataCatalogue::DataCatalogue(const int& scicatServerID): serverID(scicatServerID), serverType(static_cast<SciCat::serverType>(serverID))
{
    scicatUser = new DataCatalogueCore::User(this);
    scicatExperiment = new DataCatalogueCore::Experiment(this);
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

QJsonDocument DataCatalogue::getUserInfo(){
    scicatUser->sendUserInfoRequest();
    return scicatUser->userInfo;
}

void DataCatalogue::setPgroup(const QString& pgroup){
    scicatUser->pgroup = pgroup;
}


DataCatalogueCore::Experiment* DataCatalogue::experiment()
{
    return scicatExperiment;
}






