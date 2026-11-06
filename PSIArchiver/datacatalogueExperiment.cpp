#include "datacatalogueExperiment.h"
#include "networkmanager.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkReply>

using namespace SciCat::DataCatalogueCore;


void Experiment::setInvestigator(const QString &inv)
{
    m_investigator = inv;
}

QString Experiment::getInvestigator() const
{
    return m_investigator;
}

Experiment::Experiment(DataCatalogue * const parent) : scicatBase(parent)
{

}

QString Experiment::getDescription() const
{
    return m_description;
}

void Experiment::setDescription(const QString &description)
{
    m_description = description;
}

void Experiment::setInvestigatorToPI(bool investigatorIsPI)
{
    investigatorIsPI ? m_investigator = m_PIemail : m_investigator = scicatBase->user()->getUserMail();
}

PgroupRequestState Experiment::getPgroupInfo(){

    Q_ASSERT(!m_pgroup.isEmpty());

    QSettings settingsContainer("PSI", "SciCat");

    QJsonObject filter{{"where", QJsonObject{{"ownerGroup", m_pgroup}}}};
    QNetworkRequest request = scicatBase->apiRequest("proposals", ApiVersion::V3, filter, "filters");
    QString reply = NetworkManager::startNetworkManagerAndWait(NetworkManager::GetOperation, request);
    QJsonDocument doc = QJsonDocument::fromJson(reply.toUtf8());

    if (doc.isArray() && !doc.array().isEmpty()){
        QJsonObject ob = doc.array()[0].toObject();

        setPIFirstname(ob.value("pi_firstname").toString());
        setPILastname(ob.value("pi_lastname").toString());
        setPIemail(ob.value("pi_email").toString());
        setProposalTitle(ob.value("title").toString());
        setProposalAbstract(ob.value("abstract").toString());
        setInstrument(ob.value("MeasurementPeriodList").toArray()[0].toObject().find("instrument").value().toString());
        setProposalID(ob.value("proposalId").toString());
        m_PgroupRequestState = PgroupRequestState::ValidSciCatEntry;
        return PgroupRequestState::ValidSciCatEntry;
    } else if (settingsContainer.value("loadProposalInfo",scicatBase->baseUI->defaultSettings.loadProposalInfo).toBool()) {

        QString dirpath = settingsContainer.value("logPath", scicatBase->baseUI->defaultSettings.logPath).toString() + "/proposals/";
        QString proposalFile = QDir(dirpath).filePath("proposal_" + getPgroup() +".dat");

        QFile file(proposalFile);
        if(!file.open(QIODevice::ReadOnly)) {
            m_PgroupRequestState = PgroupRequestState::Invalid;
            return PgroupRequestState::Invalid;
        } else {
            QTextStream in(&file);

            QStringList proposalContent;
            while(!in.atEnd()) {
                proposalContent.push_back(in.readLine());
            }
            qDebug() << proposalContent.size();
            file.close();
            if (proposalContent.size()<3){
                m_PgroupRequestState = PgroupRequestState::Invalid;
                return PgroupRequestState::Invalid;
            } else {
                setPIFirstname(proposalContent[0]);
                setPILastname(proposalContent[1]);
                setPIemail(proposalContent[2]);
                setProposalID("");
                m_PgroupRequestState = PgroupRequestState::ValidLogFile;
                return PgroupRequestState::ValidLogFile;
            }
        }

    } else {
        m_PgroupRequestState = PgroupRequestState::Invalid;
        return PgroupRequestState::Invalid;
    }

}

PgroupRequestState Experiment::getPgroupRequestState(){
    return m_PgroupRequestState;
}

int Experiment::savePIInfo(){
    QSettings settingsContainer("PSI", "SciCat");

    qDebug() << "Writing PI info";
    QString dirpath = settingsContainer.value("logPath", scicatBase->baseUI->defaultSettings.logPath).toString() + "/proposals/";
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            return 2;
        }
    }

    QString proposalFile = QDir(dirpath).filePath("proposal_" + getPgroup() + ".dat");

    QFile file(proposalFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        return 1;
    } else {
        QTextStream out(&file);
        out << getPIFirstname() << "\n";
        out << getPILastname() << "\n";
        out << getPIemail() << "\n";
        file.close();
    }
    return 0;
}

QString Experiment::getOwnerEmail() const
{
    return m_ownerEmail;
}

void Experiment::setOwnerEmail(const QString &ownerEmail)
{
    m_ownerEmail = ownerEmail;
}

QString Experiment::getOwner() const
{
    return m_owner;
}

void Experiment::setOwner(const QString &owner)
{
    m_owner = owner;
}

QString Experiment::getOwnerGroup() const
{
    return m_ownerGroup;
}

void Experiment::setOwnerGroup(const QString &ownerGroup)
{
    m_ownerGroup = ownerGroup;
}

QString Experiment::getSampleDescription() const
{
    return m_sampleDescription;
}

void Experiment::setSampleDescription(const QString &sampleDescription)
{
    m_sampleDescription = sampleDescription;
}

QString Experiment::getSamplename() const
{
    return m_samplename;
}

void Experiment::setSamplename(const QString &samplename)
{
    m_samplename = samplename;
}

QString Experiment::getKeywords() const
{
    return m_keywords;
}

void Experiment::setKeywords(const QString &keywords)
{
    m_keywords = keywords;
}

QString Experiment::getUsedSoftware() const
{
    return m_usedSoftware;
}

void Experiment::setUsedSoftware(const QString &usedSoftware)
{
    m_usedSoftware = usedSoftware;
}

QString Experiment::getInputDatasets() const
{
    return m_inputDatasets;
}

void Experiment::setInputDatasets(const QString &inputDatasets)
{
    m_inputDatasets = inputDatasets;
}

QString Experiment::getCreationLocation() const
{
    return m_creationLocation;
}

void Experiment::setCreationLocation(const QString &creationLocation)
{
    m_creationLocation = creationLocation;
}

QString Experiment::getSourceFolder() const
{
    return m_sourceFolder;
}

void Experiment::setSourceFolder(const QString &sourceFolder)
{
    m_sourceFolder = sourceFolder;
}

QString Experiment::getDatasetName() const
{
    return m_datasetName;
}

void Experiment::setDatasetName(const QString &datasetName)
{
    m_datasetName = datasetName;
}

bool Experiment::getDataIsRaw() const
{
    return m_dataIsRaw;
}

void Experiment::setDataIsRaw(bool dataIsRaw)
{
    m_dataIsRaw = dataIsRaw;
}

QString Experiment::getPgroup() const
{
    return m_pgroup;
}

void Experiment::setPgroup(const QString &pgroup)
{
    m_pgroup = pgroup;
    m_ownerGroup = pgroup;
}

QString Experiment::getInstrument() const
{
    return m_instrument;
}

void Experiment::setInstrument(const QString &instrument)
{
    m_instrument = instrument;
    m_creationLocation = instrument;
}

QString Experiment::getProposalAbstract() const
{
    return m_proposalAbstract;
}

void Experiment::setProposalAbstract(const QString &proposalAbstract)
{
    m_proposalAbstract = proposalAbstract;
}

QString Experiment::getProposalTitle() const
{
    return m_proposalTitle;
}

void Experiment::setProposalTitle(const QString &proposalTitle)
{
    m_proposalTitle = proposalTitle;
}

QString Experiment::getPILastname() const
{
    return m_PILastname;
}

void Experiment::setPILastname(const QString &PILastname)
{
    m_PILastname = PILastname;
    m_owner = m_PIFirstname + " " + m_PILastname;
}

QString Experiment::getPIFirstname() const
{
    return m_PIFirstname;
}

void Experiment::setPIFirstname(const QString &PIFirstname)
{
    m_PIFirstname = PIFirstname;
    m_owner = m_PIFirstname + " " + m_PILastname;
}

QString Experiment::getPIemail() const
{
    return m_PIemail;
}

void Experiment::setPIemail(const QString &PIemail)
{
    m_PIemail = PIemail;
    m_ownerEmail = PIemail;
}

QString Experiment::getProposalID() const
{
    return m_proposalID;
}

void Experiment::setProposalID(const QString &proposalID)
{
    m_proposalID = proposalID;
}
