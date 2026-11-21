#include "datasetlistmodel.h"

#include "formatting.h"

#include <QSet>

using namespace scicat;

QVariant DatasetListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case PidRole: return row.dataset.pid;
    case NameRole: return row.dataset.name;
    case TypeRole: return row.dataset.type;
    case SizeRole: return row.dataset.sizeBytes;
    case SizeTextRole: return formatBytes(row.dataset.sizeBytes);
    case CreatedRole: return row.dataset.created;
    case CreatedTextRole: return formatDateTime(row.dataset.created);
    case FileCountRole: return row.dataset.fileCount;
    case StateRole: return row.state;
    case SelectedRole: return row.selected;
    case SourceFolderRole: return row.dataset.sourceFolder;
    case JobNoteRole: return row.jobNote;
    }
    return QVariant();
}

QHash<int, QByteArray> DatasetListModel::roleNames() const
{
    return {{PidRole, "pid"}, {NameRole, "name"}, {TypeRole, "type"}, {SizeRole, "sizeBytes"}, {SizeTextRole, "sizeText"},
            {CreatedRole, "created"}, {CreatedTextRole, "createdText"}, {FileCountRole, "fileCount"}, {StateRole, "archiveState"},
            {SelectedRole, "selected"}, {SourceFolderRole, "sourceFolder"}, {JobNoteRole, "jobNote"}};
}

void DatasetListModel::setDatasets(const QList<DatasetSummary> &datasets)
{
    // ticks belong to datasets, not to rows: they survive a reload of the same pgroup
    QSet<QString> ticked;
    for (const Row &row : std::as_const(m_rows)) {
        if (row.selected)
            ticked.insert(row.dataset.pid);
    }
    beginResetModel();
    m_rows.clear();
    m_index.clear();
    m_selectedCount = 0;
    m_stateCounts.fill(0);
    m_selectedInState.fill(0);
    m_rows.reserve(datasets.size());
    for (const DatasetSummary &dataset : datasets) {
        const State state = dataset.retrievable ? InArchive : NotArchived;
        const bool selected = ticked.contains(dataset.pid);
        m_index.insert(dataset.pid, int(m_rows.size()));
        m_rows.append({dataset, state, selected, QString()});
        ++m_stateCounts[state];
        if (selected) {
            ++m_selectedCount;
            ++m_selectedInState[state];
        }
    }
    endResetModel();
    emit selectionChanged();
    emit statesChanged();
}

bool DatasetListModel::setRowState(int row, State state, const QString &note)
{
    Row &entry = m_rows[row];
    if (entry.state == state && entry.jobNote == note)
        return false;
    if (entry.state != state) {
        --m_stateCounts[entry.state];
        ++m_stateCounts[state];
        if (entry.selected) {
            --m_selectedInState[entry.state];
            ++m_selectedInState[state];
        }
        entry.state = state;
    }
    entry.jobNote = note;
    emit dataChanged(index(row), index(row), {StateRole, JobNoteRole});
    return true;
}

void DatasetListModel::applyJobs(const QList<RetrieveJob> &jobs)
{
    // per dataset: a finished job wins (the data is on the retrieve server), then a waiting one, then a failed one
    QHash<QString, const RetrieveJob *> succeeded, pending, failed;
    for (const RetrieveJob &job : jobs) {
        auto &target = job.succeeded() ? succeeded : job.pending() ? pending : failed;
        for (const QString &pid : job.pids) {
            if (!target.contains(pid)) // jobs arrive newest first
                target.insert(pid, &job);
        }
    }
    bool changed = false;
    for (int row = 0; row < m_rows.size(); ++row) {
        const Row &entry = m_rows.at(row);
        const QString &pid = entry.dataset.pid;
        State state = entry.dataset.retrievable ? InArchive : NotArchived;
        QString note;
        if (const RetrieveJob *job = succeeded.value(pid)) {
            state = Ready;
            note = tr("Retrieved %1").arg(formatDateTime(job->created));
        } else if (const RetrieveJob *job = pending.value(pid)) {
            state = Requested;
            note = tr("Requested %1").arg(formatDateTime(job->created));
        } else if (const RetrieveJob *job = failed.value(pid)) {
            if (entry.dataset.retrievable)
                state = RequestFailed;
            note = job->statusMessage.isEmpty() ? tr("The request of %1 failed").arg(formatDateTime(job->created))
                                                : tr("The request of %1 failed: %2").arg(formatDateTime(job->created), job->statusMessage);
        }
        changed = setRowState(row, state, note) || changed;
    }
    // a poll that finds nothing new must not make the views do anything
    if (changed)
        emit statesChanged();
}

void DatasetListModel::markRequested(const QStringList &pids)
{
    bool changed = false;
    for (const QString &pid : pids) {
        const int row = rowOf(pid);
        if (row >= 0 && m_rows.at(row).state != Ready)
            changed = setRowState(row, Requested, tr("Requested just now")) || changed;
    }
    if (changed)
        emit statesChanged();
}

void DatasetListModel::setRowsSelected(const QList<int> &rows, bool selected)
{
    int first = -1;
    int last = -1;
    for (int row : rows) {
        if (row < 0 || row >= m_rows.size() || m_rows.at(row).selected == selected)
            continue;
        Row &entry = m_rows[row];
        entry.selected = selected;
        m_selectedCount += selected ? 1 : -1;
        m_selectedInState[entry.state] += selected ? 1 : -1;
        first = first < 0 ? row : qMin(first, row);
        last = qMax(last, row);
    }
    if (first < 0)
        return;
    emit dataChanged(index(first), index(last), {SelectedRole});
    emit selectionChanged();
}

void DatasetListModel::clearSelection()
{
    if (m_selectedCount > 0)
        setRowsSelected(selectedRows(), false);
}

QList<int> DatasetListModel::selectedRows() const
{
    QList<int> rows;
    rows.reserve(m_selectedCount);
    for (int row = 0; row < m_rows.size(); ++row) {
        if (m_rows.at(row).selected)
            rows << row;
    }
    return rows;
}

QString DatasetListModel::stateText(State state)
{
    switch (state) {
    case NotArchived: return tr("Not in archive");
    case InArchive: return tr("In archive");
    case Requested: return tr("Requested");
    case Ready: return tr("Ready to copy");
    case RequestFailed: return tr("Request failed");
    }
    return QString();
}

DatasetFilterModel::DatasetFilterModel(DatasetListModel *source, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_source(source)
{
    setSourceModel(source);
    // A state change reaches the proxy as dataChanged of that row. With dynamic sorting and filtering on, the row
    // then enters or leaves the list or moves to its new place by itself; no reset, so the view keeps its position.
    setDynamicSortFilter(true);
    sort(0, Qt::DescendingOrder);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DatasetFilterModel::scheduleCountChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DatasetFilterModel::scheduleCountChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &DatasetFilterModel::scheduleCountChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &DatasetFilterModel::scheduleCountChanged);
}

void DatasetFilterModel::scheduleCountChanged()
{
    if (m_countPending)
        return;
    m_countPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_countPending = false;
        emit countChanged();
    }, Qt::QueuedConnection);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
QSortFilterProxyModel::DataChangeRelevanceFlags DatasetFilterModel::dataChangeRelevanceFlags(const QModelIndex &, const QModelIndex &,
                                                                                             const QList<int> &roles) const
{
    if (roles.isEmpty())
        return DataChangeRelevanceFlag::RelevantForFilteringAndSorting;
    // a tick or a new note never moves a row; a new state does while the list is filtered or ordered by state
    DataChangeRelevanceFlags relevance = DataChangeRelevanceFlag::NotRelevant;
    if (roles.contains(DatasetListModel::StateRole)) {
        if (m_stateFilter >= 0)
            relevance |= DataChangeRelevanceFlag::RelevantForFiltering;
        if (m_sortKey == ByState)
            relevance |= DataChangeRelevanceFlag::RelevantForSorting;
    }
    return relevance;
}
#endif

void DatasetFilterModel::setSearch(const QString &search)
{
    const QString trimmed = search.trimmed();
    if (m_search == trimmed)
        return;
    changeFilter([&] { m_search = trimmed; });
    emit filterChanged();
    scheduleCountChanged();
}

void DatasetFilterModel::setStateFilter(int state)
{
    if (m_stateFilter == state)
        return;
    changeFilter([&] { m_stateFilter = state; });
    emit filterChanged();
    scheduleCountChanged();
}

void DatasetFilterModel::sortBy(SortKey key)
{
    if (m_sortKey == key) {
        m_descending = !m_descending;
    } else {
        m_sortKey = key;
        m_descending = key == BySize || key == ByCreated; // largest and newest first, names from A
    }
    // one pass: sort() re-sorts by itself when the order turns around, and has to be told when only the key changed
    const Qt::SortOrder order = m_descending ? Qt::DescendingOrder : Qt::AscendingOrder;
    if (order != sortOrder())
        sort(0, order);
    else
        invalidate();
    emit sortChanged();
}

void DatasetFilterModel::clearFilters()
{
    if (!filtered())
        return;
    changeFilter([&] {
        m_search.clear();
        m_stateFilter = -1;
    });
    emit filterChanged();
    scheduleCountChanged();
}

int DatasetFilterModel::sourceRow(int row) const
{
    return mapToSource(index(row, 0)).row();
}

int DatasetFilterModel::rowOfPid(const QString &pid) const
{
    const int source = m_source->rowOf(pid);
    return source < 0 ? -1 : mapFromSource(m_source->index(source)).row();
}

QString DatasetFilterModel::pidAt(int row) const
{
    const int source = sourceRow(row);
    return source < 0 ? QString() : m_source->dataset(source).pid;
}

bool DatasetFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    if (m_stateFilter >= 0 && int(m_source->state(sourceRow)) != m_stateFilter)
        return false;
    if (m_search.isEmpty())
        return true;
    const DatasetSummary &dataset = m_source->dataset(sourceRow);
    return dataset.name.contains(m_search, Qt::CaseInsensitive) || dataset.pid.contains(m_search, Qt::CaseInsensitive);
}

bool DatasetFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const DatasetSummary &a = m_source->dataset(left.row());
    const DatasetSummary &b = m_source->dataset(right.row());
    switch (m_sortKey) {
    case ByState:
        if (m_source->state(left.row()) != m_source->state(right.row()))
            return m_source->state(left.row()) < m_source->state(right.row());
        break;
    case BySize:
        if (a.sizeBytes != b.sizeBytes)
            return a.sizeBytes < b.sizeBytes;
        break;
    case ByCreated:
        if (a.created != b.created)
            return a.created < b.created;
        break;
    case ByName:
        break;
    }
    const int byName = QString::localeAwareCompare(a.name, b.name);
    return byName != 0 ? byName < 0 : a.pid < b.pid;
}
