#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include "datacatalogueClasses.h"

namespace SciCat {
namespace DataCatalogueCore {

enum class PgroupRequestState {ValidSciCatEntry, ValidLogFile, Invalid};

class Experiment
{
public:
    void setInvestigator(const QString& investigator);
    QString getInvestigator() const;


    QString getProposalID() const;
    void setProposalID(const QString &proposalID);

    QString getPIemail() const;
    void setPIemail(const QString &PIemail);

    QString getPIFirstname() const;
    void setPIFirstname(const QString &PIFirstname);

    QString getPILastname() const;
    void setPILastname(const QString &PILastname);

    QString getProposalTitle() const;
    void setProposalTitle(const QString &proposalTitle);

    QString getProposalAbstract() const;
    void setProposalAbstract(const QString &proposalAbstract);

    QString getInstrument() const;
    void setInstrument(const QString &instrument);

    QString getPgroup() const;
    void setPgroup(const QString &pgroup);

    bool getDataIsRaw() const;
    void setDataIsRaw(bool dataIsRaw);

    QString getDatasetName() const;
    void setDatasetName(const QString &datasetName);

    QString getSourceFolder() const;
    void setSourceFolder(const QString &sourceFolder);

    QString getCreationLocation() const;
    void setCreationLocation(const QString &creationLocation);

    QString getInputDatasets() const;
    void setInputDatasets(const QString &inputDatasets);

    QString getUsedSoftware() const;
    void setUsedSoftware(const QString &usedSoftware);

    QString getKeywords() const;
    void setKeywords(const QString &keywords);

    QString getSamplename() const;
    void setSamplename(const QString &samplename);

    QString getSampleDescription() const;
    void setSampleDescription(const QString &sampleDescription);

    QString getOwnerGroup() const;
    void setOwnerGroup(const QString &ownerGroup);

    QString getOwner() const;
    void setOwner(const QString &owner);

    QString getOwnerEmail() const;
    void setOwnerEmail(const QString &ownerEmail);

    QString getDescription() const;
    void setDescription(const QString &description);

    void setInvestigatorToPI(bool investigatorIsPI);

    PgroupRequestState getPgroupInfo();

    PgroupRequestState getPgroupRequestState();

    int savePIInfo();

private:
    Experiment(DataCatalogue * const parent);
    friend class SciCat::DataCatalogue;
    DataCatalogue * const scicatBase;

    QString m_investigator;
    QString m_PIemail;
    QString m_PIFirstname;
    QString m_PILastname;
    QString m_proposalID;
    QString m_proposalTitle;
    QString m_proposalAbstract;
    QString m_instrument;
    QString m_pgroup;
    bool m_dataIsRaw; // raw or derived
    QString m_datasetName;
    QString m_sourceFolder;
    QString m_creationLocation;
    QString m_inputDatasets;
    QString m_usedSoftware;
    QString m_keywords;
    QString m_samplename;
    QString m_sampleDescription;
    QString m_ownerGroup;
    QString m_owner;
    QString m_ownerEmail;
    QString m_description;
    PgroupRequestState m_PgroupRequestState;

};


}
}


#endif // EXPERIMENT_H
