#include "activitymodel.h"

#include "copyqueue.h"
#include "formatting.h"
#include "ingestrun.h"
#include "retrievecontroller.h"

#include <QDir>
#include <QSet>

using namespace scicat;

ActivityModel::ActivityModel(CopyQueue *copies, RetrieveController *retrieve, QObject *parent)
    : QAbstractListModel(parent)
    , m_copies(copies)
    , m_retrieve(retrieve)
{
    connect(copies, &CopyQueue::taskAdded, this, [this](CopyTask *task) {
        connect(task, &CopyTask::logChanged, this, [this, task] { rowChanged(task, {LogRole}); });
        connect(task, &CopyTask::changed, this, [this, task] {
            rowChanged(task, {StateRole, StateTextRole, DetailRole, CanCancelRole});
            emit runningCountChanged();
        });
    });
    connect(copies, &CopyQueue::tasksChanged, this, &ActivityModel::rebuild);
    connect(retrieve, &RetrieveController::jobsChanged, this, &ActivityModel::rebuild);
    // a request is named after its first dataset, and the names arrive with the dataset list
    connect(retrieve, &RetrieveController::countsChanged, this, [this] {
        if (!m_rows.isEmpty())
            emit dataChanged(index(0), index(int(m_rows.size()) - 1), {TitleRole});
    });
}

QVariant ActivityModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const Row &row = m_rows.at(index.row());
    const State state = stateOf(row);
    switch (role) {
    case KindRole: return row.kind;
    case IdRole: return idOf(row);
    case LocalRole: return row.kind != Request;
    case StateRole: return state;
    case TitleRole:
        if (row.kind == Ingest && row.ingest)
            return row.ingest->dryRun() ? tr("Check %1").arg(row.ingest->datasetName()) : tr("Archive %1").arg(row.ingest->datasetName());
        if (row.kind == Copy && row.copy)
            return tr("Copy %1").arg(row.copy->name().isEmpty() ? row.copy->pid() : row.copy->name());
        if (row.kind == Request) {
            const QString first = m_retrieve->datasetName(row.job.pids.value(0));
            if (row.job.pids.size() == 1)
                return tr("Request %1").arg(first.isEmpty() ? row.job.pids.value(0) : first);
            return first.isEmpty() ? tr("Request %1 datasets").arg(row.job.pids.size())
                                   : tr("Request %1 and %2 more").arg(first).arg(row.job.pids.size() - 1);
        }
        return QString();
    case DetailRole:
        if (row.kind == Ingest && row.ingest)
            return row.ingest->active() ? tr("To %1 · running for %2").arg(row.ingest->serverLabel(), formatDuration(row.ingest->elapsedSeconds()))
                                        : row.ingest->resultText();
        if (row.kind == Copy && row.copy)
            return row.copy->active() ? tr("To %1").arg(QDir::toNativeSeparators(row.copy->folder())) : row.copy->resultText();
        if (row.kind == Request)
            return row.job.statusMessage;
        return QString();
    case StateTextRole:
        switch (state) {
        case Waiting: return row.kind == Request ? tr("Waiting in the archive") : tr("Queued");
        case Running: return row.kind == Request ? tr("Retrieving from tape") : tr("Running");
        case Done: return row.kind == Request ? tr("Ready to copy") : tr("Done");
        case Failed: return tr("Failed");
        case Cancelled: return tr("Stopped");
        }
        return QString();
    case TimeTextRole:
        if (row.kind == Ingest && row.ingest)
            return formatDateTime(row.ingest->startedAt());
        if (row.kind == Copy && row.copy)
            return formatDateTime(row.copy->createdAt());
        return formatDateTime(row.job.created);
    case LogRole:
        return row.kind == Ingest && row.ingest ? row.ingest->log() : row.kind == Copy && row.copy ? row.copy->log() : QString();
    case CanCancelRole:
        return (row.kind == Ingest && row.ingest && row.ingest->active()) || (row.kind == Copy && row.copy && row.copy->active());
    case FolderRole:
        if (row.kind == Copy && row.copy)
            return row.copy->folder();
        if (row.kind == Ingest && row.ingest)
            return row.ingest->logFolder();
        return QString();
    }
    return QVariant();
}

QHash<int, QByteArray> ActivityModel::roleNames() const
{
    return {{KindRole, "kind"}, {TitleRole, "title"}, {DetailRole, "detail"}, {StateRole, "taskState"}, {StateTextRole, "stateText"},
            {TimeTextRole, "timeText"}, {LogRole, "log"}, {CanCancelRole, "canCancel"}, {FolderRole, "folder"}, {LocalRole, "local"},
            {IdRole, "taskId"}};
}

// The same task keeps its id for as long as it is listed, whatever row it is on.
QString ActivityModel::idOf(const Row &row)
{
    if (row.kind == Ingest)
        return QStringLiteral("ingest:%1").arg(quintptr(row.ingest.data()));
    if (row.kind == Copy)
        return QStringLiteral("copy:%1").arg(quintptr(row.copy.data()));
    return QStringLiteral("request:") + row.job.id;
}

bool ActivityModel::lists(const IngestRun *run) const
{
    for (const QPointer<IngestRun> &listed : m_ingests) {
        if (listed == run)
            return true;
    }
    return false;
}

ActivityModel::State ActivityModel::stateOf(const Row &row) const
{
    if (row.kind == Ingest && row.ingest) {
        switch (row.ingest->state()) {
        case IngestRun::Preparing:
        case IngestRun::Running: return Running;
        case IngestRun::Succeeded: return Done;
        case IngestRun::Failed: return Failed;
        case IngestRun::Cancelled: return Cancelled;
        }
    }
    if (row.kind == Copy && row.copy) {
        switch (row.copy->state()) {
        case CopyTask::Queued: return Waiting;
        case CopyTask::Running: return Running;
        case CopyTask::Succeeded: return Done;
        case CopyTask::Failed: return Failed;
        case CopyTask::Cancelled: return Cancelled;
        }
    }
    if (row.job.succeeded())
        return Done;
    if (row.job.failed())
        return Failed;
    return row.job.statusCode == QLatin1String("inProgress") ? Running : Waiting;
}

int ActivityModel::runningCount() const
{
    int running = m_copies->activeCount();
    for (const QPointer<IngestRun> &run : m_ingests)
        running += run && run->active();
    return running;
}

int ActivityModel::finishedLocalCount() const
{
    int finished = 0;
    for (const Row &row : m_rows)
        finished += (row.kind == Ingest && row.ingest && !row.ingest->active()) || (row.kind == Copy && row.copy && !row.copy->active());
    return finished;
}

void ActivityModel::addIngest(IngestRun *run)
{
    m_ingests.prepend(run);
    connect(run, &IngestRun::logChanged, this, [this, run] { rowChanged(run, {LogRole}); });
    connect(run, &IngestRun::elapsedChanged, this, [this, run] { rowChanged(run, {DetailRole}); });
    connect(run, &IngestRun::stateChanged, this, [this, run] {
        rowChanged(run, {StateRole, StateTextRole, DetailRole, CanCancelRole, FolderRole});
        emit runningCountChanged();
        emit countChanged();
    });
    rebuild();
}

void ActivityModel::cancel(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    if (m_rows.at(row).ingest)
        m_rows.at(row).ingest->cancel();
    if (m_rows.at(row).copy)
        m_rows.at(row).copy->cancel();
}

void ActivityModel::clearFinished()
{
    for (int i = int(m_ingests.size()) - 1; i >= 0; --i) {
        if (!m_ingests.at(i) || !m_ingests.at(i)->active())
            m_ingests.removeAt(i);
    }
    m_copies->removeFinished();
    rebuild();
}

void ActivityModel::stopAll()
{
    for (const QPointer<IngestRun> &run : std::as_const(m_ingests)) {
        if (run)
            run->cancel();
    }
    m_copies->cancelAll();
}

void ActivityModel::forgetSession()
{
    // the tasks were told to stop before this; nothing of the old session stays listed, also what is still stopping
    m_ingests.clear();
    m_copies->forgetAll();
    rebuild();
}

// Brings the rows in line with the tasks and requests that exist now. Rows that stay are kept, so a view keeps its
// place, its open log and its focus; only when the order itself changed is the list built again.
void ActivityModel::rebuild()
{
    // this session's work first, newest on top, then what SciCat knows
    QList<Row> next;
    for (const QPointer<IngestRun> &run : std::as_const(m_ingests)) {
        if (run)
            next.append({Ingest, run, nullptr, {}});
    }
    for (CopyTask *task : m_copies->tasks())
        next.append({Copy, nullptr, task, {}});
    std::stable_sort(next.begin(), next.end(), [](const Row &a, const Row &b) {
        const QDateTime ta = a.ingest ? a.ingest->startedAt() : a.copy->createdAt();
        const QDateTime tb = b.ingest ? b.ingest->startedAt() : b.copy->createdAt();
        return ta > tb;
    });
    const QList<RetrieveJob> &jobs = m_retrieve->jobs();
    for (int i = 0; i < jobs.size() && i < 200; ++i)
        next.append({Request, nullptr, nullptr, jobs.at(i)});

    QStringList nextIds;
    for (const Row &row : std::as_const(next))
        nextIds << idOf(row);
    const QSet<QString> wanted(nextIds.cbegin(), nextIds.cend());

    // 1. rows that are gone
    for (int row = int(m_rows.size()) - 1; row >= 0; --row) {
        if (!wanted.contains(idOf(m_rows.at(row)))) {
            beginRemoveRows(QModelIndex(), row, row);
            m_rows.removeAt(row);
            endRemoveRows();
        }
    }
    // 2. the rows that stay must come in the order they will have; if not, start over
    QStringList keptIds;
    for (const Row &row : std::as_const(m_rows))
        keptIds << idOf(row);
    QStringList expected;
    const QSet<QString> kept(keptIds.cbegin(), keptIds.cend());
    for (const QString &id : std::as_const(nextIds)) {
        if (kept.contains(id))
            expected << id;
    }
    if (expected != keptIds) {
        beginResetModel();
        m_rows = next;
        endResetModel();
    } else {
        // 3. new rows go in where they belong; rows that stay take over what changed about their request
        for (int row = 0; row < next.size(); ++row) {
            if (row < m_rows.size() && idOf(m_rows.at(row)) == nextIds.at(row)) {
                if (!(m_rows.at(row).job == next.at(row).job)) {
                    m_rows[row] = next.at(row);
                    emit dataChanged(index(row), index(row));
                }
                continue;
            }
            beginInsertRows(QModelIndex(), row, row);
            m_rows.insert(row, next.at(row));
            endInsertRows();
        }
    }
    emit countChanged();
    emit runningCountChanged();
}

void ActivityModel::rowChanged(const QObject *object, const QList<int> &roles)
{
    for (int row = 0; row < m_rows.size(); ++row) {
        if (m_rows.at(row).ingest == object || m_rows.at(row).copy == object) {
            emit dataChanged(index(row), index(row), roles);
            return;
        }
    }
}
