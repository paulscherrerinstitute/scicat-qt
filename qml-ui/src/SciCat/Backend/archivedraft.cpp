#include "archivedraft.h"

#include "appsettings.h"
#include "formatting.h"
#include "session.h"
#include "settings.h"
#include "workspace.h"

#include <QDir>

using namespace scicat;

namespace {
// creationLocationList of the Widgets GUI (PSIArchiver/dialog.h)
const QStringList builtInInstruments{
    "/PSI/SLS/ADRESS-RIXS", "/PSI/SLS/ADRESS-SX-ARPES", "/PSI/SLS/CSAXS", "/PSI/SLS/MICRO-XAS", "/PSI/SLS/MICRO-XAS-FEMTO",
    "/PSI/SLS/MS-POWDER", "/PSI/SLS/MS-SURF-DIFFR", "/PSI/SLS/NANOXAS", "/PSI/SLS/PEARL", "/PSI/SLS/PHOENIX", "/PSI/SLS/POLLUX",
    "/PSI/SLS/MX", "/PSI/SLS/SIM", "/PSI/SLS/SIS-COPHEE", "/PSI/SLS/SIS-HRPES", "/PSI/SLS/SUPER-XAS", "/PSI/SLS/TOMCAT",
    "/PSI/SLS/VUV", "/PSI/SLS/XIL-II", "/PSI/SLS/XTREME", "/PSI/SWISSFEL/ARAMIS-ALVRA", "/PSI/SWISSFEL/ARAMIS-BERNINA",
    "/PSI/SWISSFEL/ARAMIS-CRISTALLINA", "/PSI/SINQ/AMOR", "/PSI/SINQ/DMC", "/PSI/SINQ/EIGER", "/PSI/SINQ/FOCUS", "/PSI/SINQ/HRPT",
    "/PSI/SINQ/ICON", "/PSI/SINQ/MORPHEUS", "/PSI/SINQ/NARZISS", "/PSI/SINQ/NEUTRA", "/PSI/SINQ/POLDI", "/PSI/SINQ/RITA-II",
    "/PSI/SINQ/SANS-I", "/PSI/SINQ/SANS-II", "/PSI/SINQ/TASP", "/PSI/SINQ/ZEBRA", "/PSI/SMUS/DOLLY", "/PSI/SMUS/GPD", "/PSI/SMUS/GPS",
    "/PSI/SMUS/HAL-9500", "/PSI/SMUS/LEM", "/PSI/SMUS/LTF", "/PSI/RA", "/PSI/MERLIN"};
} // namespace

ArchiveDraft::ArchiveDraft(ApiClient *api, Session *session, Workspace *workspace, Settings *settings, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_session(session)
    , m_workspace(workspace)
    , m_settings(settings)
    , m_files(new FileEntryModel(this))
    , m_metadata(new MetadataModel(this))
    , m_inputDatasets(new InputDatasetModel(this))
    , m_pickerSource(new DatasetListModel(this))
    , m_picker(new DatasetFilterModel(m_pickerSource, this))
    , m_instruments(builtInInstruments)
{
    m_metadata->resetToTemplate();
    m_picker->sortBy(DatasetFilterModel::ByName);
    applyOptionDefaults();

    connect(m_files, &FileEntryModel::countChanged, this, &ArchiveDraft::problemsChanged);
    connect(m_files, &FileEntryModel::totalsChanged, this, &ArchiveDraft::problemsChanged);
    connect(m_files, &FileEntryModel::countChanged, this, &ArchiveDraft::contentChanged);
    connect(m_metadata, &MetadataModel::contentChanged, this, &ArchiveDraft::problemsChanged);
    connect(this, &ArchiveDraft::detailsChanged, this, &ArchiveDraft::problemsChanged);
    connect(this, &ArchiveDraft::detailsChanged, this, &ArchiveDraft::contentChanged);
    connect(m_pickerSource, &DatasetListModel::selectionChanged, this, &ArchiveDraft::pickedCountChanged);
    connect(workspace, &Workspace::piChanged, this, &ArchiveDraft::problemsChanged);
    // What was typed over the proposal's values belongs to that proposal. Another pgroup, another PI or a proposal
    // loaded again start from the proposal, as the Widgets GUI does when its pgroup page is passed again.
    connect(workspace, &Workspace::pgroupChanged, this, &ArchiveDraft::forgetOwnership);
    connect(workspace, &Workspace::proposalChanged, this, &ArchiveDraft::forgetOwnership);
    connect(workspace, &Workspace::piChanged, this, [this] {
        // the owner follows the PI's name, the two addresses follow the PI's email
        m_ownerTyped = QString();
        m_ownerEmailTyped = QString();
        m_principalInvestigatorTyped = QString();
        emit ownershipChanged();
    });
    // "Register by default" and "Send to tape by default" are the starting values of the two boxes of the review
    // step: a change in the settings reaches a dataset whose boxes nobody has touched yet
    connect(settings, &Settings::changed, this, [this] {
        if (!m_optionsTouched && (m_ingest != m_settings->enableIngest() || m_autoArchive != m_settings->enableArchive()))
            applyOptionDefaults();
    });
    connect(session, &Session::userChanged, this, &ArchiveDraft::ownershipChanged);
    connect(this, &ArchiveDraft::ownershipChanged, this, &ArchiveDraft::problemsChanged);
    connect(workspace, &Workspace::stateChanged, this, [this] {
        if (m_workspace->groupReady())
            applyGroupDefaults();
        emit problemsChanged();
    });
    connect(workspace, &Workspace::pgroupChanged, this, [this] {
        // input datasets and the list they were picked from belong to the previous pgroup
        m_inputDatasets->clear();
        m_pickerSource->clear();
        m_pickerState = PickerIdle;
        m_pickerNote.clear();
        emit pickerChanged();
    });
}

void ArchiveDraft::setStep(int step)
{
    step = qBound(int(FilesStep), step, int(ReviewStep));
    if (m_step == step)
        return;
    m_step = step;
    if (step == DetailsStep) {
        applyGroupDefaults();
        // suggest a name from what was selected, unless the user typed one
        if (m_settings->guessDatasetName() && m_datasetName.isEmpty() && !m_nameEdited) {
            m_datasetName = normalizedDatasetName(guessDatasetName(m_files->paths()));
            emit detailsChanged();
        }
    }
    emit stepChanged();
}

void ArchiveDraft::setRaw(bool raw)
{
    if (m_raw == raw)
        return;
    m_raw = raw;
    emit detailsChanged();
}

void ArchiveDraft::setDatasetName(const QString &name)
{
    const QString normalized = normalizedDatasetName(name.trimmed());
    m_nameEdited = true;
    if (m_datasetName == normalized)
        return;
    m_datasetName = normalized;
    emit detailsChanged();
}

void ArchiveDraft::setDescription(const QString &description)
{
    if (m_description == description)
        return;
    m_description = description;
    emit detailsChanged();
}

void ArchiveDraft::setCreationLocation(const QString &location)
{
    if (m_creationLocation == location)
        return;
    m_creationLocation = location;
    emit detailsChanged();
}

void ArchiveDraft::setInvestigator(const QString &investigator)
{
    if (m_investigator == investigator)
        return;
    m_investigator = investigator;
    emit detailsChanged();
}

QString ArchiveDraft::owner() const
{
    return m_ownerTyped.isNull() ? m_workspace->owner() : m_ownerTyped;
}

void ArchiveDraft::setOwner(const QString &owner)
{
    if (owner == this->owner())
        return;
    m_ownerTyped = owner.isNull() ? QStringLiteral("") : owner;
    emit ownershipChanged();
}

QString ArchiveDraft::ownerEmail() const
{
    return m_ownerEmailTyped.isEmpty() ? m_workspace->defaultOwnerEmail() : m_ownerEmailTyped;
}

void ArchiveDraft::setOwnerEmail(const QString &email)
{
    // an emptied field, or the proposal's own value typed again, is "as the proposal says"
    const QString typed = email.trimmed() == m_workspace->defaultOwnerEmail() ? QString() : email.trimmed();
    if (typed == m_ownerEmailTyped)
        return;
    m_ownerEmailTyped = typed;
    emit ownershipChanged();
}

QString ArchiveDraft::ownerGroup() const
{
    return m_ownerGroupTyped.isEmpty() ? m_workspace->pgroup() : m_ownerGroupTyped;
}

void ArchiveDraft::setOwnerGroup(const QString &group)
{
    const QString typed = group.trimmed() == m_workspace->pgroup() ? QString() : group.trimmed();
    if (typed == m_ownerGroupTyped)
        return;
    m_ownerGroupTyped = typed;
    emit ownershipChanged();
}

QString ArchiveDraft::principalInvestigator() const
{
    return m_principalInvestigatorTyped.isEmpty() ? m_workspace->defaultPrincipalInvestigator() : m_principalInvestigatorTyped;
}

void ArchiveDraft::setPrincipalInvestigator(const QString &investigator)
{
    const QString typed = investigator.trimmed() == m_workspace->defaultPrincipalInvestigator() ? QString() : investigator.trimmed();
    if (typed == m_principalInvestigatorTyped)
        return;
    m_principalInvestigatorTyped = typed;
    emit ownershipChanged();
}

QString ArchiveDraft::proposalId() const
{
    return m_proposalIdTyped.isNull() ? m_workspace->proposalId() : m_proposalIdTyped;
}

void ArchiveDraft::setProposalId(const QString &id)
{
    // an emptied proposal ID stays empty: a dataset does not have to belong to a proposal
    const QString typed = id.trimmed();
    if (typed == proposalId())
        return;
    m_proposalIdTyped = typed.isNull() ? QStringLiteral("") : typed;
    emit ownershipChanged();
}

void ArchiveDraft::forgetOwnership()
{
    m_ownerTyped = QString();
    m_ownerEmailTyped = QString();
    m_ownerGroupTyped = QString();
    m_principalInvestigatorTyped = QString();
    m_proposalIdTyped = QString();
    emit ownershipChanged();
}

void ArchiveDraft::setUsedSoftware(const QString &software)
{
    if (m_usedSoftware == software)
        return;
    m_usedSoftware = software;
    emit detailsChanged();
}

void ArchiveDraft::setIngest(bool ingest)
{
    if (m_ingest == ingest)
        return;
    m_ingest = ingest;
    m_optionsTouched = true;
    emit optionsChanged();
}

void ArchiveDraft::setAutoArchive(bool autoArchive)
{
    if (m_autoArchive == autoArchive)
        return;
    m_autoArchive = autoArchive;
    m_optionsTouched = true;
    emit optionsChanged();
}

QString ArchiveDraft::filesProblem() const
{
    if (m_files->count() == 0)
        return m_shown[FilesStep] ? tr("Add at least one file or folder.") : QString();
    if (m_files->missingCount() > 0)
        return tr("%1 of the entries no longer exist. Remove them from the list.").arg(m_files->missingCount());
    if (!m_files->scanning() && m_files->overLimit())
        return tr("%1 files are selected. A dataset can hold fewer than %2: remove some entries and archive them as a second dataset.")
            .arg(formatCount(m_files->totalFiles()), formatCount(AppSettings::maxFilesPerDataset));
    return QString();
}

QString ArchiveDraft::datasetNameProblem() const
{
    return m_shown[DetailsStep] && m_datasetName.isEmpty() ? tr("Give the dataset a name.") : QString();
}

QString ArchiveDraft::instrumentProblem() const
{
    return m_shown[DetailsStep] && m_raw && m_creationLocation.isEmpty() ? tr("Choose the instrument the data was measured at.") : QString();
}

QString ArchiveDraft::investigatorProblem() const
{
    return m_shown[DetailsStep] && !m_raw && m_investigator.trimmed().isEmpty() ? tr("Enter the email address of the investigator.") : QString();
}

// The three checks of the Widgets GUI (Dialog::checkMetadata), said at the fields instead of in a box.
QString ArchiveDraft::ownerProblem() const
{
    return m_shown[DetailsStep] && owner().trimmed().isEmpty() ? tr("Enter the owner of the dataset.") : QString();
}

QString ArchiveDraft::ownerEmailProblem() const
{
    // also before "Next": only a typed address can be wrong, and that is worth saying at once
    return !m_ownerEmailTyped.isEmpty() && !isEmailAddress(m_ownerEmailTyped) ? tr("This is not an email address.") : QString();
}

QString ArchiveDraft::ownerGroupProblem() const
{
    return m_session->groups().contains(ownerGroup()) ? QString() : tr("The owner group has to be one of your pgroups.");
}

QString ArchiveDraft::metadataProblem() const
{
    return m_shown[DetailsStep] && m_raw && m_metadata->hasProblems() ? tr("Some scientific metadata entries need a name or a valid number.") : QString();
}

QVariantList ArchiveDraft::problems() const
{
    QVariantList list;
    const auto add = [&list](int step, const QString &text) { list.append(QVariantMap{{QStringLiteral("step"), step}, {QStringLiteral("text"), text}}); };
    if (!m_workspace->groupReady())
        add(-1, tr("Choose a pgroup."));
    else if (!m_workspace->piComplete())
        add(-1, tr("The PI details of %1 are incomplete.").arg(m_workspace->pgroup()));
    if (m_files->count() == 0)
        add(FilesStep, tr("Add at least one file or folder."));
    else if (m_files->missingCount() > 0)
        add(FilesStep, tr("Some entries no longer exist."));
    else if (!m_files->scanning() && m_files->overLimit())
        add(FilesStep, tr("Too many files for one dataset."));
    if (m_datasetName.isEmpty())
        add(DetailsStep, tr("Give the dataset a name."));
    if (m_raw && m_creationLocation.isEmpty())
        add(DetailsStep, tr("Choose the instrument."));
    if (!m_raw && m_investigator.trimmed().isEmpty())
        add(DetailsStep, tr("Enter the investigator."));
    if (owner().trimmed().isEmpty())
        add(DetailsStep, tr("Enter the owner."));
    if (!ownerEmailProblem().isEmpty())
        add(DetailsStep, tr("The owner email is not an email address."));
    if (m_workspace->groupReady() && !ownerGroupProblem().isEmpty())
        add(DetailsStep, tr("The owner group has to be one of your pgroups."));
    if (m_raw && m_metadata->hasProblems())
        add(DetailsStep, tr("Fix the scientific metadata entries."));
    return list;
}

bool ArchiveDraft::hasContent() const
{
    return m_files->count() > 0 || !m_datasetName.isEmpty() || !m_description.isEmpty();
}

bool ArchiveDraft::checkStep(int step)
{
    if (step < FilesStep || step > ReviewStep)
        return false;
    if (!m_shown[step]) {
        m_shown[step] = true;
        emit problemsChanged();
    }
    return stepOk(step);
}

bool ArchiveDraft::stepOk(int step) const
{
    switch (step) {
    case FilesStep:
        return m_files->count() > 0 && m_files->missingCount() == 0 && (m_files->scanning() || !m_files->overLimit());
    case DetailsStep:
        return !m_datasetName.isEmpty() && (m_raw ? !m_creationLocation.isEmpty() && !m_metadata->hasProblems() : !m_investigator.trimmed().isEmpty())
               && !owner().trimmed().isEmpty() && ownerEmailProblem().isEmpty() && ownerGroupProblem().isEmpty();
    default:
        return problems().isEmpty();
    }
}

QString ArchiveDraft::addInstrument(const QString &institute, const QString &facility, const QString &instrument)
{
    QString missing;
    const QString location = customCreationLocation(institute, facility, instrument, &missing);
    if (location.isEmpty())
        return missing;
    if (!m_instruments.contains(location)) {
        m_instruments.append(location);
        emit instrumentsChanged();
    }
    setCreationLocation(location);
    return QString();
}

QUrl ArchiveDraft::pickerStartFolder() const
{
    const QString group = m_workspace->pgroup();
    const QDir groupFolder(QStringLiteral("/das/work/") + group.mid(0, 3) + QLatin1Char('/') + group);
    if (!group.isEmpty() && groupFolder.exists() && m_settings->usePgroupAsDefaultPath())
        return QUrl::fromLocalFile(groupFolder.path());
    return QUrl::fromLocalFile(QDir::homePath());
}

void ArchiveDraft::loadPicker()
{
    const QString group = m_workspace->pgroup();
    if (group.isEmpty() || m_pickerState == PickerLoading || (m_pickerState == PickerReady && m_pickerGroup == group))
        return;
    m_pickerGroup = group;
    m_pickerState = PickerLoading;
    m_pickerNote.clear();
    emit pickerChanged();
    m_pickerFetch = m_api->fetchAllPages(QStringLiteral("datasets"), ApiVersion::V3, datasetPickFilter(group), datasetListLimits());
    connect(m_pickerFetch, &PagedFetch::finished, this, [this, group](PagedFetch *fetch) {
        if (group != m_workspace->pgroup())
            return;
        if (fetch->status() == PagedFetch::Status::Failed) {
            m_pickerState = PickerFailed;
            m_pickerNote = tr("Could not load the datasets of %1 from SciCat. %2").arg(group, fetch->errorText());
            emit pickerChanged();
            return;
        }
        QList<DatasetSummary> datasets;
        for (const QJsonValue &item : fetch->items())
            datasets << parseDataset(item.toObject());
        m_pickerSource->setDatasets(datasets);
        m_pickerState = PickerReady;
        if (fetch->status() == PagedFetch::Status::Capped)
            m_pickerNote = tr("Only the first %1 datasets of this pgroup are listed.").arg(formatCount(m_api->pageSize() * m_api->maxPages()));
        emit pickerChanged();
    });
}

// Every time the dialog opens it starts clean, and the list is asked for again: a dataset registered a minute ago,
// here or by someone else, has to be there to be picked.
void ArchiveDraft::openPicker()
{
    m_pickerSource->clearSelection();
    m_picker->clearFilters();
    if (m_pickerState == PickerLoading)
        return;
    m_pickerState = PickerIdle;
    loadPicker();
}

void ArchiveDraft::togglePicked(int row)
{
    const int source = m_picker->sourceRow(row);
    if (source >= 0)
        m_pickerSource->setSelected(source, !m_pickerSource->selected(source));
}

int ArchiveDraft::addPicked()
{
    int added = 0;
    const QList<int> rows = m_pickerSource->selectedRows();
    for (int row : rows)
        added += m_inputDatasets->add(m_pickerSource->dataset(row).pid, m_pickerSource->dataset(row).name);
    m_pickerSource->clearSelection();
    return added;
}

bool ArchiveDraft::submit()
{
    if (m_run && m_run->active())
        return false;
    for (int step = FilesStep; step <= ReviewStep; ++step)
        m_shown[step] = true;
    emit problemsChanged();
    if (!problems().isEmpty())
        return false;

    const QString warning = m_workspace->rememberPi();
    IngestRun::Request request;
    request.draft = snapshot();
    request.ingest = m_ingest;
    request.autoArchive = m_autoArchive && m_ingest; // a dry run archives nothing
    request.serverId = m_session->serverId();
    request.serverLabel = m_session->environmentLabel();
    request.token = m_session->token();
    m_run = new IngestRun(request, m_files, m_settings, this);
    emit runChanged();
    emit runStarted(m_run);
    m_run->start();
    if (!warning.isEmpty())
        emit this->warning(warning);
    return true;
}

// Sign-out: nothing of this user's dataset stays, also while its run is still being stopped. The run is detached
// here and ends on its own.
void ArchiveDraft::forgetSession()
{
    m_run = nullptr;
    startNew(false);
}

void ArchiveDraft::startNew(bool keepDetails)
{
    if (m_run && m_run->active())
        return;
    m_files->clear();
    m_datasetName.clear();
    m_nameEdited = false;
    m_inputDatasets->clear();
    if (!keepDetails) {
        m_description.clear();
        m_usedSoftware.clear();
        m_creationLocation.clear();
        m_investigator.clear();
        m_autoInvestigator.clear();
        m_autoLocation.clear();
        m_raw = true;
        m_metadata->resetToTemplate();
        applyOptionDefaults();
        forgetOwnership();
    }
    for (bool &shown : m_shown)
        shown = false;
    m_run = nullptr;
    m_step = FilesStep;
    if (m_workspace->groupReady())
        applyGroupDefaults();
    emit runChanged();
    emit stepChanged();
    emit detailsChanged();
    emit problemsChanged();
    emit contentChanged();
    emit startedNew();
}

DatasetDraft ArchiveDraft::snapshot() const
{
    DatasetDraft draft;
    draft.raw = m_raw;
    draft.datasetName = m_datasetName;
    draft.description = m_description;
    draft.owner = owner();
    draft.ownerGroup = ownerGroup();
    draft.ownerEmail = ownerEmail();
    draft.proposalId = proposalId();
    draft.principalInvestigator = principalInvestigator();
    draft.creationLocation = m_creationLocation;
    draft.scientificMetadata = m_metadata->toJson();
    draft.investigator = m_investigator;
    draft.usedSoftware = m_usedSoftware;
    draft.inputDatasets = m_inputDatasets->pids();
    draft.paths = m_files->paths();
    return draft;
}

QString ArchiveDraft::defaultInvestigator() const
{
    return m_settings->investigatorIsPI() ? principalInvestigator() : m_session->email();
}

void ArchiveDraft::applyGroupDefaults()
{
    bool changed = false;
    // the investigator follows the preference until the user types another one
    const QString investigator = defaultInvestigator();
    if ((m_investigator.isEmpty() || m_investigator == m_autoInvestigator) && m_investigator != investigator) {
        m_investigator = investigator;
        changed = true;
    }
    m_autoInvestigator = investigator;

    // the instrument of the proposal is the starting point, also when it is not in the built-in list
    const QString location = m_workspace->instrument();
    if (!location.isEmpty() && !m_instruments.contains(location)) {
        m_instruments.append(location);
        emit instrumentsChanged();
    }
    if ((m_creationLocation.isEmpty() || m_creationLocation == m_autoLocation) && m_creationLocation != location) {
        m_creationLocation = location;
        changed = true;
    }
    m_autoLocation = location;
    if (changed)
        emit detailsChanged();
}

void ArchiveDraft::applyOptionDefaults()
{
    m_ingest = m_settings->enableIngest();
    m_autoArchive = m_settings->enableArchive();
    m_optionsTouched = false;
    emit optionsChanged();
}
