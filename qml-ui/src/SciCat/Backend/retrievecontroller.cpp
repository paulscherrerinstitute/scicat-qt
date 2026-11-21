#include "retrievecontroller.h"

#include "copyqueue.h"
#include "formatting.h"
#include "session.h"
#include "settings.h"
#include "workspace.h"

#include <QDateTime>
#include <QDir>

using namespace scicat;

RetrieveController::RetrieveController(ApiClient *api, Session *session, Workspace *workspace, Settings *settings,
                                       CopyQueue *copies, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_session(session)
    , m_workspace(workspace)
    , m_settings(settings)
    , m_copies(copies)
    , m_source(new DatasetListModel(this))
    , m_filter(new DatasetFilterModel(m_source, this))
    , m_details(new DatasetDetails(api, this))
{
    connect(m_source, &DatasetListModel::selectionChanged, this, &RetrieveController::selectionChanged);
    connect(m_source, &DatasetListModel::statesChanged, this, &RetrieveController::selectionChanged);
    connect(m_source, &DatasetListModel::statesChanged, this, &RetrieveController::countsChanged);
    connect(settings, &Settings::changed, this, &RetrieveController::selectionChanged);
    // "none, some or all of the listed rows are ticked" also changes when the filter lists other rows
    connect(this, &RetrieveController::selectionChanged, this, &RetrieveController::filteredSelectionChanged);
    connect(m_filter, &DatasetFilterModel::countChanged, this, &RetrieveController::filteredSelectionChanged);
    // the current dataset can be on another row, or on none, after any of these
    for (auto moved : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved})
        connect(m_filter, moved, this, &RetrieveController::currentRowChanged);
    connect(m_filter, &QAbstractItemModel::rowsMoved, this, &RetrieveController::currentRowChanged);
    connect(m_filter, &QAbstractItemModel::layoutChanged, this, &RetrieveController::currentRowChanged);
    connect(m_filter, &QAbstractItemModel::modelReset, this, &RetrieveController::currentRowChanged);
    connect(this, &RetrieveController::currentPidChanged, this, &RetrieveController::currentRowChanged);

    connect(workspace, &Workspace::pgroupChanged, this, &RetrieveController::forgetGroup);
    connect(session, &Session::aboutToSignOut, this, &RetrieveController::forgetUser);
    // The retrieve requests are asked for when a page needs them (Retrieve with its list, Activity when it is
    // opened), and from then on for as long as one of them is waiting.
    connect(&m_poll, &QTimer::timeout, this, [this] { loadJobs(false); });
    m_detailsDelay.setSingleShot(true);
    m_detailsDelay.setInterval(150);
    connect(&m_detailsDelay, &QTimer::timeout, this, &RetrieveController::showCurrentDetails);
}

QStringList RetrieveController::warnings() const
{
    return m_shownJobsWarning.isEmpty() ? m_warnings : m_warnings + QStringList{m_shownJobsWarning};
}

QList<int> RetrieveController::stateCounts() const
{
    QList<int> counts;
    for (int state = DatasetListModel::NotArchived; state <= DatasetListModel::RequestFailed; ++state)
        counts << m_source->countInState(DatasetListModel::State(state));
    return counts;
}

bool RetrieveController::requestable(int sourceRow) const
{
    switch (m_source->state(sourceRow)) {
    case DatasetListModel::InArchive:
    case DatasetListModel::RequestFailed:
        return true;
    case DatasetListModel::Ready:
        return m_settings->requestAlreadyRetrieved();
    default:
        return false;
    }
}

// The three numbers below come from counts the list keeps as rows change, so reading them costs nothing.
int RetrieveController::requestableCount() const
{
    return m_source->selectedInState(DatasetListModel::InArchive) + m_source->selectedInState(DatasetListModel::RequestFailed)
           + (m_settings->requestAlreadyRetrieved() ? m_source->selectedInState(DatasetListModel::Ready) : 0);
}

int RetrieveController::copyableCount() const
{
    return m_source->selectedInState(DatasetListModel::Ready);
}

int RetrieveController::filteredSelection() const
{
    const int listed = m_filter->rowCount();
    const int selected = m_source->selectedCount();
    if (selected == 0 || listed == 0)
        return 0;
    if (!m_filter->filtered())
        return selected == listed ? 2 : 1;
    int listedSelected = 0;
    const QList<int> rows = m_source->selectedRows();
    for (int row : rows)
        listedSelected += m_filter->lists(row);
    return listedSelected == 0 ? 0 : listedSelected == listed ? 2 : 1;
}

QString RetrieveController::selectionHint() const
{
    if (selectedCount() == 0)
        return QString();
    const int notArchived = m_source->selectedInState(DatasetListModel::NotArchived);
    const int requested = m_source->selectedInState(DatasetListModel::Requested);
    QStringList parts;
    if (notArchived > 0)
        parts << tr("%1 not in the archive yet").arg(notArchived);
    if (requested > 0)
        parts << tr("%1 already requested").arg(requested);
    return parts.join(tr(", "));
}

void RetrieveController::setCurrentPid(const QString &pid)
{
    if (m_currentPid == pid)
        return;
    m_currentPid = pid;
    // One click shows its dataset at once. While the user runs through the list with a key held down, only the
    // dataset they stop on is loaded: every other one would start two requests just to abort them.
    const bool inQuickSuccession = m_sinceCurrentChange.isValid() && m_sinceCurrentChange.elapsed() < 250;
    m_sinceCurrentChange.start();
    if (m_source->rowOf(pid) >= 0 && inQuickSuccession) {
        m_detailsDelay.start();
    } else {
        m_detailsDelay.stop();
        showCurrentDetails();
    }
    emit currentPidChanged();
}

void RetrieveController::showCurrentDetails()
{
    const int row = m_source->rowOf(m_currentPid);
    if (row < 0)
        m_details->clear();
    else
        m_details->show(m_source->dataset(row), m_source->state(row), m_source->data(m_source->index(row), DatasetListModel::JobNoteRole).toString());
}

bool RetrieveController::copySupported() const
{
#ifdef Q_OS_WIN
    return false;
#else
    return true;
#endif
}

QString RetrieveController::datasetName(const QString &pid) const
{
    const int row = m_source->rowOf(pid);
    return row < 0 ? QString() : m_source->dataset(row).name;
}

void RetrieveController::ensureLoaded()
{
    if (!m_workspace->groupReady())
        return;
    if (m_loadState == Idle || m_loadedGroup != m_workspace->pgroup())
        reload();
}

void RetrieveController::reload()
{
    if (!m_workspace->groupReady() || !m_session->signedIn())
        return;
    // abort() only schedules the fetch for deletion, so the pointers have to be cleared here: loadJobs() takes a
    // fetch that is still set as "one is running" and would send nothing
    if (m_jobsFetch)
        m_jobsFetch->abort();
    if (m_datasetsFetch)
        m_datasetsFetch->abort();
    m_jobsFetch = nullptr;
    m_datasetsFetch = nullptr;
    m_loadedGroup = m_workspace->pgroup();
    m_loadState = Loading;
    m_loadedCount = 0;
    m_errorText.clear();
    m_warnings.clear();
    emit loadedCountChanged();
    emit loadStateChanged();
    loadJobs(true);
}

void RetrieveController::refreshJobs()
{
    loadJobs(false);
}

void RetrieveController::ensureJobsLoaded()
{
    if (!m_jobsAsked)
        loadJobs(false);
}

// Another pgroup: its datasets go, and with them the selection, the filters and what was in flight for them.
// The retrieve requests stay, they are the user's.
void RetrieveController::forgetGroup()
{
    if (m_datasetsFetch)
        m_datasetsFetch->abort();
    m_datasetsFetch = nullptr;
    if (m_requestReply) {
        // the answer would clear the selection and set a notice in a list it no longer belongs to
        disconnect(m_requestReply, nullptr, this, nullptr);
        m_requestReply = nullptr;
    }
    if (m_requesting) {
        m_requesting = false;
        emit requestingChanged();
    }
    stopTicketCheck();
    m_detailsDelay.stop();
    m_source->clear();
    m_filter->clearFilters();
    m_details->forget();
    m_currentPid.clear();
    m_loadedGroup.clear();
    m_loadState = Idle;
    m_errorText.clear();
    m_warnings.clear();
    m_loadedCount = 0;
    dismissNotice();
    emit currentPidChanged();
    emit loadStateChanged();
    emit loadedCountChanged();
    emit countsChanged();
    updatePolling();
}

void RetrieveController::forgetUser()
{
    m_poll.stop();
    if (m_jobsFetch)
        m_jobsFetch->abort();
    m_jobsFetch = nullptr;
    forgetGroup();
    m_jobsAsked = false;
    m_jobsWarning.clear();
    m_shownJobsWarning.clear();
    if (!m_jobs.isEmpty()) {
        m_jobs.clear();
        emit jobsChanged();
    }
    emit loadStateChanged();
}

void RetrieveController::loadJobs(bool thenDatasets)
{
    if (m_jobsFetch || !m_session->signedIn())
        return;
    m_jobsAsked = true;
    const QString username = m_session->username();
    // the retrieve jobs of this user, newest first; the v3 jobs endpoint rejects any "where" filter, so v4
    m_jobsFetch = m_api->fetchAllPages(QStringLiteral("jobs"), ApiVersion::V4, retrieveJobsFilter(username), retrieveJobsLimits());
    connect(m_jobsFetch, &PagedFetch::finished, this, [this, username, thenDatasets](PagedFetch *fetch) {
        m_jobsFetch = nullptr; // it deletes itself; a load asked for from here on must not see it as running
        QList<RetrieveJob> jobs;
        for (const QJsonValue &item : fetch->items()) {
            if (isRetrieveJobOf(item.toObject(), username))
                jobs << parseRetrieveJob(item.toObject());
        }
        m_jobsWarning.clear();
        if (fetch->status() == PagedFetch::Status::Failed) {
            // an answer that did not come does not count as asked: the next page that needs the jobs asks again
            m_jobsAsked = false;
            m_jobsWarning = tr("Could not load all retrieve jobs from SciCat, so requested or retrieved datasets may show as \"In archive\".");
        } else if (fetch->status() == PagedFetch::Status::Capped) {
            m_jobsWarning = tr("Only the most recent %1 retrieve jobs were checked, so older retrieved datasets may show as \"In archive\".")
                                .arg(formatCount(m_api->pageSize() * m_api->maxPages()));
        }
        // a failed refresh keeps what the last complete answer said
        if ((fetch->status() != PagedFetch::Status::Failed || thenDatasets) && jobs != m_jobs) {
            m_jobs = jobs;
            emit jobsChanged();
        }
        if (thenDatasets) {
            loadDatasets();
            return;
        }
        applyJobs();
    });
}

void RetrieveController::loadDatasets()
{
    const QString group = m_loadedGroup;
    // all datasets of the pgroup
    m_datasetsFetch = m_api->fetchAllPages(QStringLiteral("datasets"), ApiVersion::V3, datasetListFilter(group), datasetListLimits());
    connect(m_datasetsFetch, &PagedFetch::progress, this, [this](int items) {
        m_loadedCount = items;
        emit loadedCountChanged();
    });
    connect(m_datasetsFetch, &PagedFetch::finished, this, [this, group](PagedFetch *fetch) {
        m_datasetsFetch = nullptr;
        if (group != m_workspace->pgroup())
            return;
        if (fetch->status() == PagedFetch::Status::Failed && fetch->items().isEmpty()) {
            m_errorText = fetch->httpStatus() == 0
                              ? tr("Could not reach SciCat to list the datasets of %1.").arg(group)
                              : tr("SciCat did not list the datasets of %1 (HTTP %2): %3").arg(group).arg(fetch->httpStatus()).arg(fetch->errorText());
            m_loadState = Failed;
            emit loadStateChanged();
            return;
        }
        QList<DatasetSummary> datasets;
        datasets.reserve(fetch->items().size());
        for (const QJsonValue &item : fetch->items())
            datasets << parseDataset(item.toObject());
        m_source->setDatasets(datasets);
        if (fetch->status() == PagedFetch::Status::Failed)
            m_warnings << tr("Could not load all datasets of this pgroup from SciCat. The list is incomplete.");
        else if (fetch->status() == PagedFetch::Status::Capped)
            m_warnings << tr("Only the first %1 datasets of this pgroup are listed.").arg(formatCount(m_api->pageSize() * m_api->maxPages()));
        m_loadState = Loaded;
        applyJobs();
        if (m_source->rowOf(m_currentPid) < 0)
            setCurrentPid(QString());
        emit countsChanged();
        emit loadStateChanged();
    });
}

void RetrieveController::applyJobs()
{
    m_source->applyJobs(m_jobs);
    if (m_shownJobsWarning != m_jobsWarning) {
        m_shownJobsWarning = m_jobsWarning;
        emit loadStateChanged();
    }
    const int row = m_source->rowOf(m_currentPid);
    if (row >= 0 && !m_detailsDelay.isActive())
        m_details->updateState(m_source->state(row), m_source->data(m_source->index(row), DatasetListModel::JobNoteRole).toString());
    updatePolling();
}

void RetrieveController::updatePolling()
{
    bool waiting = m_source->countInState(DatasetListModel::Requested) > 0;
    for (const RetrieveJob &job : std::as_const(m_jobs))
        waiting = waiting || job.pending();
    if (!waiting || !m_session->signedIn()) {
        m_poll.stop();
        return;
    }
    // look often right after a request made here, then settle to twice a minute
    const bool recent = QDateTime::currentSecsSinceEpoch() - m_lastRequestAt < 90;
    const int interval = recent ? 5000 : 30000;
    if (!m_poll.isActive() || m_poll.interval() != interval)
        m_poll.start(interval);
}

void RetrieveController::toggle(int row)
{
    const int source = m_filter->sourceRow(row);
    if (source >= 0)
        m_source->setSelected(source, !m_source->selected(source));
}

void RetrieveController::setListedSelected(bool selected)
{
    QList<int> rows;
    rows.reserve(m_filter->rowCount());
    for (int row = 0; row < m_filter->rowCount(); ++row)
        rows << m_filter->sourceRow(row);
    m_source->setRowsSelected(rows, selected);
}

void RetrieveController::clearSelection()
{
    m_source->clearSelection();
}

void RetrieveController::requestSelected()
{
    if (m_requesting)
        return;
    QStringList pids;
    int notArchived = 0, alreadyRequested = 0, alreadyRetrieved = 0;
    const QList<int> rows = m_source->selectedRows();
    for (int row : rows) {
        if (requestable(row)) {
            pids << m_source->dataset(row).pid;
            continue;
        }
        switch (m_source->state(row)) {
        case DatasetListModel::NotArchived: ++notArchived; break;
        case DatasetListModel::Requested: ++alreadyRequested; break;
        default: ++alreadyRetrieved; break;
        }
    }
    QStringList skipped;
    if (notArchived > 0)
        skipped << tr("%1 not in the archive yet").arg(notArchived);
    if (alreadyRequested > 0)
        skipped << tr("%1 already requested").arg(alreadyRequested);
    if (alreadyRetrieved > 0)
        skipped << tr("%1 already retrieved").arg(alreadyRetrieved);
    const QString skippedText = skipped.isEmpty() ? QString() : tr(" Skipped: %1.").arg(skipped.join(tr(", ")));
    if (pids.isEmpty()) {
        setNotice(QStringLiteral("warning"), tr("Nothing was requested.") + skippedText);
        return;
    }

    // start retrieving from the archive; at this point the data is only copied to the retrieve server
    QNetworkRequest request = m_api->request(QStringLiteral("jobs"), ApiVersion::V4);
    request.setRawHeader("Content-Type", "application/json");
    const QByteArray body = QJsonDocument(retrieveRequestBody(m_session->username(), m_session->email(), m_workspace->pgroup(), pids))
                                .toJson(QJsonDocument::Compact);
    m_requesting = true;
    emit requestingChanged();
    m_requestReply = m_api->post(request, body);
    connect(m_requestReply, &ApiReply::finished, this, [this, pids, skippedText](ApiReply *answer) {
        m_requestReply = nullptr;
        m_requesting = false;
        emit requestingChanged();
        if (answer->connectionFailed()) {
            setNotice(QStringLiteral("error"), tr("Could not connect to SciCat: %1. Nothing was requested.").arg(answer->errorString()));
            return;
        }
        if (!answer->ok()) {
            setNotice(QStringLiteral("error"), tr("SciCat rejected the retrieve request (HTTP %1): %2").arg(answer->httpStatus()).arg(answer->serverMessage()));
            return;
        }
        m_lastRequestAt = QDateTime::currentSecsSinceEpoch();
        m_source->markRequested(pids);
        m_source->clearSelection();
        setNotice(QStringLiteral("success"),
                  (pids.size() == 1 ? tr("Requested 1 dataset from the archive.") : tr("Requested %1 datasets from the archive.").arg(pids.size()))
                      + tr(" SciCat emails you when the data is ready, and this list updates by itself.") + skippedText);
        // SciCat answers with the job it made: it is listed at once, without asking for all jobs again, and what
        // becomes of it is asked from now on
        RetrieveJob created = parseRetrieveJob(answer->json().object());
        if (!created.id.isEmpty()) {
            if (created.pids.isEmpty())
                created.pids = pids;
            if (created.statusCode.isEmpty())
                created.statusCode = QStringLiteral("jobSubmitted");
            m_jobs.prepend(created);
            emit jobsChanged();
        }
        updatePolling();
    });
}

void RetrieveController::checkTicket()
{
    if (m_ticketCheck)
        return;
    // if klist cannot answer (missing, hanging or crashing) the copy goes ahead and scicat-cli reports a missing ticket itself
    m_ticketCheck = new QProcess(this);
    emit checkingTicketChanged();
    const auto done = [this](bool hasTicket) {
        if (!m_ticketCheck)
            return;
        stopTicketCheck();
        emit ticketChecked(hasTicket);
    };
    connect(m_ticketCheck, &QProcess::errorOccurred, this, [done](QProcess::ProcessError) { done(true); });
    connect(m_ticketCheck, &QProcess::finished, this, [done](int exitCode, QProcess::ExitStatus status) {
        done(status != QProcess::NormalExit || exitCode == 0);
    });
    QTimer::singleShot(3000, m_ticketCheck, [this, done] {
        if (m_ticketCheck) {
            m_ticketCheck->kill();
            done(true);
        }
    });
    m_ticketCheck->start(QStringLiteral("klist"), {QStringLiteral("-s")});
}

void RetrieveController::stopTicketCheck()
{
    if (!m_ticketCheck)
        return;
    m_ticketCheck->disconnect(this);
    if (m_ticketCheck->state() != QProcess::NotRunning)
        m_ticketCheck->kill();
    m_ticketCheck->deleteLater();
    m_ticketCheck = nullptr;
    emit checkingTicketChanged();
}

void RetrieveController::copySelectedTo(const QUrl &folderUrl)
{
    const QString folder = folderUrl.toLocalFile();
    if (folder.isEmpty())
        return;
    int queued = 0, notReady = 0;
    const QList<int> rows = m_source->selectedRows();
    for (int row : rows) {
        if (m_source->state(row) != DatasetListModel::Ready) {
            ++notReady;
            continue;
        }
        m_copies->enqueue(m_source->dataset(row).pid, m_source->dataset(row).name, folder);
        ++queued;
    }
    const QString skipped = notReady == 1 ? tr(" 1 selected dataset is not ready to copy and was left out.")
                            : notReady > 1 ? tr(" %1 selected datasets are not ready to copy and were left out.").arg(notReady) : QString();
    if (queued == 0) {
        setNotice(QStringLiteral("warning"), tr("Nothing is being copied.") + skipped);
        return;
    }
    m_source->clearSelection();
    setNotice(QStringLiteral("info"),
              (queued == 1 ? tr("Copying 1 dataset to %1.").arg(QDir::toNativeSeparators(folder))
                           : tr("Copying %1 datasets to %2.").arg(queued).arg(QDir::toNativeSeparators(folder)))
                  + tr(" Follow the progress under Activity.") + skipped);
}

void RetrieveController::explainNothingToCopy()
{
    setNotice(QStringLiteral("warning"), tr("None of the selected datasets is ready to copy yet. Request them first: a dataset can be copied once its state is \"Ready to copy\"."));
}

void RetrieveController::dismissNotice()
{
    setNotice(QString(), QString());
}

void RetrieveController::setNotice(const QString &tone, const QString &text)
{
    if (m_noticeTone == tone && m_noticeText == text)
        return;
    m_noticeTone = tone;
    m_noticeText = text;
    emit noticeChanged();
}
