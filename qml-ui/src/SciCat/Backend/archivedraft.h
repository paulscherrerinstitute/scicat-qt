#pragma once

#include "apiclient.h"
#include "datasetlistmodel.h"
#include "fileentrymodel.h"
#include "ingestrun.h"
#include "inputdatasetmodel.h"
#include "metadatamodel.h"

#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Session;
class Settings;
class Workspace;

// The dataset being prepared for the archive: files, details, and the run that sends it.
class ArchiveDraft : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.archive")

    Q_PROPERTY(FileEntryModel *files READ files CONSTANT)
    Q_PROPERTY(MetadataModel *metadata READ metadata CONSTANT)
    Q_PROPERTY(InputDatasetModel *inputDatasets READ inputDatasets CONSTANT)
    Q_PROPERTY(DatasetFilterModel *picker READ picker CONSTANT)
    Q_PROPERTY(PickerState pickerState READ pickerState NOTIFY pickerChanged)
    Q_PROPERTY(QString pickerNote READ pickerNote NOTIFY pickerChanged)
    Q_PROPERTY(int pickedCount READ pickedCount NOTIFY pickedCountChanged)
    Q_PROPERTY(IngestRun *run READ run NOTIFY runChanged)
    Q_PROPERTY(int step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(bool raw READ raw WRITE setRaw NOTIFY detailsChanged)
    Q_PROPERTY(QString datasetName READ datasetName WRITE setDatasetName NOTIFY detailsChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY detailsChanged)
    Q_PROPERTY(QString creationLocation READ creationLocation WRITE setCreationLocation NOTIFY detailsChanged)
    Q_PROPERTY(QString investigator READ investigator WRITE setInvestigator NOTIFY detailsChanged)
    Q_PROPERTY(QString usedSoftware READ usedSoftware WRITE setUsedSoftware NOTIFY detailsChanged)
    // Who the dataset belongs to. Filled in from the proposal; each can be changed for one dataset, as in the
    // Widgets GUI. An emptied owner email or principal investigator goes back to what the proposal gives.
    Q_PROPERTY(QString owner READ owner WRITE setOwner NOTIFY ownershipChanged)
    Q_PROPERTY(QString ownerEmail READ ownerEmail WRITE setOwnerEmail NOTIFY ownershipChanged)
    Q_PROPERTY(QString ownerGroup READ ownerGroup WRITE setOwnerGroup NOTIFY ownershipChanged)
    Q_PROPERTY(QString principalInvestigator READ principalInvestigator WRITE setPrincipalInvestigator NOTIFY ownershipChanged)
    Q_PROPERTY(QString proposalId READ proposalId WRITE setProposalId NOTIFY ownershipChanged)
    Q_PROPERTY(QStringList instruments READ instruments NOTIFY instrumentsChanged)
    Q_PROPERTY(bool ingest READ ingest WRITE setIngest NOTIFY optionsChanged)
    Q_PROPERTY(bool autoArchive READ autoArchive WRITE setAutoArchive NOTIFY optionsChanged)
    Q_PROPERTY(QString filesProblem READ filesProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString datasetNameProblem READ datasetNameProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString instrumentProblem READ instrumentProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString investigatorProblem READ investigatorProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString ownerProblem READ ownerProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString ownerEmailProblem READ ownerEmailProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString ownerGroupProblem READ ownerGroupProblem NOTIFY problemsChanged)
    Q_PROPERTY(QString metadataProblem READ metadataProblem NOTIFY problemsChanged)
    Q_PROPERTY(bool detailProblemsShown READ detailProblemsShown NOTIFY problemsChanged)
    Q_PROPERTY(QVariantList problems READ problems NOTIFY problemsChanged)
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY contentChanged)

public:
    enum PickerState { PickerIdle, PickerLoading, PickerReady, PickerFailed };
    Q_ENUM(PickerState)
    enum Step { FilesStep, DetailsStep, ReviewStep };
    Q_ENUM(Step)

    ArchiveDraft(scicat::ApiClient *api, Session *session, Workspace *workspace, Settings *settings, QObject *parent = nullptr);

    FileEntryModel *files() const { return m_files; }
    MetadataModel *metadata() const { return m_metadata; }
    InputDatasetModel *inputDatasets() const { return m_inputDatasets; }
    DatasetFilterModel *picker() const { return m_picker; }
    PickerState pickerState() const { return m_pickerState; }
    QString pickerNote() const { return m_pickerNote; }
    int pickedCount() const { return m_pickerSource->selectedCount(); }
    IngestRun *run() const { return m_run; }
    int step() const { return m_step; }
    void setStep(int step);
    bool raw() const { return m_raw; }
    void setRaw(bool raw);
    QString datasetName() const { return m_datasetName; }
    void setDatasetName(const QString &name);
    QString description() const { return m_description; }
    void setDescription(const QString &description);
    QString creationLocation() const { return m_creationLocation; }
    void setCreationLocation(const QString &location);
    QString investigator() const { return m_investigator; }
    void setInvestigator(const QString &investigator);
    QString usedSoftware() const { return m_usedSoftware; }
    void setUsedSoftware(const QString &software);
    QString owner() const;
    void setOwner(const QString &owner);
    QString ownerEmail() const;
    void setOwnerEmail(const QString &email);
    QString ownerGroup() const;
    void setOwnerGroup(const QString &group);
    QString principalInvestigator() const;
    void setPrincipalInvestigator(const QString &investigator);
    QString proposalId() const;
    void setProposalId(const QString &id);
    QStringList instruments() const { return m_instruments; }
    bool ingest() const { return m_ingest; }
    void setIngest(bool ingest);
    bool autoArchive() const { return m_autoArchive; }
    void setAutoArchive(bool autoArchive);
    QString filesProblem() const;
    QString datasetNameProblem() const;
    QString instrumentProblem() const;
    QString investigatorProblem() const;
    QString ownerProblem() const;
    QString ownerEmailProblem() const;
    QString ownerGroupProblem() const;
    QString metadataProblem() const;
    bool detailProblemsShown() const { return m_shown[DetailsStep]; }
    QVariantList problems() const;
    bool hasContent() const;

    // from now on the problems of this step are shown; true when it has none
    Q_INVOKABLE bool checkStep(int step);
    // "/INSTITUTE[/FACILITY]/INSTRUMENT" is added and chosen; returns "institute" or "instrument" when that part is missing
    Q_INVOKABLE QString addInstrument(const QString &institute, const QString &facility, const QString &instrument);
    Q_INVOKABLE QUrl pickerStartFolder() const;
    Q_INVOKABLE void loadPicker();
    // for the dialog: no ticks or search left from last time, and the list as SciCat has it now
    Q_INVOKABLE void openPicker();
    Q_INVOKABLE void togglePicked(int row);
    Q_INVOKABLE int addPicked();
    Q_INVOKABLE bool submit();
    // a fresh dataset; keepDetails leaves description, type, instrument and metadata for the next one
    Q_INVOKABLE void startNew(bool keepDetails);
    void forgetSession();

    scicat::DatasetDraft snapshot() const;

signals:
    void stepChanged();
    void detailsChanged();
    void ownershipChanged();
    void instrumentsChanged();
    void optionsChanged();
    void problemsChanged();
    void contentChanged();
    void runChanged();
    void pickerChanged();
    void pickedCountChanged();
    void runStarted(IngestRun *run);
    void startedNew();
    void warning(const QString &text);

private:
    QString defaultInvestigator() const;
    void forgetOwnership();
    void applyGroupDefaults();
    void applyOptionDefaults();
    bool stepOk(int step) const;

    scicat::ApiClient *m_api;
    Session *m_session;
    Workspace *m_workspace;
    Settings *m_settings;
    FileEntryModel *m_files;
    MetadataModel *m_metadata;
    InputDatasetModel *m_inputDatasets;
    DatasetListModel *m_pickerSource;
    DatasetFilterModel *m_picker;
    PickerState m_pickerState = PickerIdle;
    QString m_pickerNote;
    QString m_pickerGroup;
    QPointer<scicat::PagedFetch> m_pickerFetch;
    QPointer<IngestRun> m_run;
    int m_step = FilesStep;
    bool m_raw = true;
    QString m_datasetName;
    QString m_description;
    QString m_creationLocation;
    QString m_investigator;
    QString m_usedSoftware;
    QStringList m_instruments;
    bool m_ingest = true;
    bool m_autoArchive = true;
    bool m_optionsTouched = false; // the two were set for this dataset; until then they follow the preferences
    bool m_shown[3] = {false, false, false};
    bool m_nameEdited = false;
    QString m_autoInvestigator;
    QString m_autoLocation;
    // what was typed over the values of the proposal; a null string means "as the proposal says"
    QString m_ownerTyped;
    QString m_ownerEmailTyped;
    QString m_ownerGroupTyped;
    QString m_principalInvestigatorTyped;
    QString m_proposalIdTyped;
};
