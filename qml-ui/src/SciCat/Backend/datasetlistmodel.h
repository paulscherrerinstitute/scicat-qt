#pragma once

#include "catalog.h"

#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QtQml/qqmlregistration.h>
#include <array>

// The datasets of a pgroup with what can be done with each of them.
class DatasetListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by the backend")

public:
    // where a dataset is on its way back from the archive
    enum State {
        NotArchived,   // not in the archive (yet): cannot be requested
        InArchive,     // can be requested from tape
        Requested,     // a retrieve job is waiting or running
        Ready,         // a retrieve job finished: can be copied to disk
        RequestFailed  // the last retrieve job failed: can be requested again
    };
    Q_ENUM(State)
    enum Roles { PidRole = Qt::UserRole + 1, NameRole, TypeRole, SizeRole, SizeTextRole, CreatedRole, CreatedTextRole,
                 FileCountRole, StateRole, SelectedRole, SourceFolderRole, JobNoteRole };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDatasets(const QList<scicat::DatasetSummary> &datasets);
    void clear() { setDatasets({}); }
    // derives every dataset's state from the user's retrieve jobs (newest first)
    void applyJobs(const QList<scicat::RetrieveJob> &jobs);
    void markRequested(const QStringList &pids);

    int rowOf(const QString &pid) const { return m_index.value(pid, -1); }
    const scicat::DatasetSummary &dataset(int row) const { return m_rows.at(row).dataset; }
    State state(int row) const { return m_rows.at(row).state; }
    bool selected(int row) const { return m_rows.at(row).selected; }
    void setSelected(int row, bool selected) { setRowsSelected({row}, selected); }
    // one notification for the whole set, so that ticking thousands of rows costs one pass
    void setRowsSelected(const QList<int> &rows, bool selected);
    void clearSelection();
    int selectedCount() const { return m_selectedCount; }
    QList<int> selectedRows() const;
    // both are kept up to date as rows change, not counted on demand
    int countInState(State state) const { return m_stateCounts[state]; }
    int selectedInState(State state) const { return m_selectedInState[state]; }

    static QString stateText(State state);

signals:
    void selectionChanged();
    void statesChanged();

private:
    struct Row
    {
        scicat::DatasetSummary dataset;
        State state = NotArchived;
        bool selected = false;
        QString jobNote;
    };
    bool setRowState(int row, State state, const QString &note);

    QList<Row> m_rows;
    QHash<QString, int> m_index;
    int m_selectedCount = 0;
    std::array<int, RequestFailed + 1> m_stateCounts{};
    std::array<int, RequestFailed + 1> m_selectedInState{};
};

// Search, state filter and sorting on top of the dataset list.
class DatasetFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by the backend")

    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)
    Q_PROPERTY(int stateFilter READ stateFilter WRITE setStateFilter NOTIFY filterChanged)
    Q_PROPERTY(bool filtered READ filtered NOTIFY filterChanged)
    Q_PROPERTY(SortKey sortKey READ sortKey NOTIFY sortChanged)
    Q_PROPERTY(bool descending READ descending NOTIFY sortChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum SortKey { ByName, ByState, BySize, ByCreated };
    Q_ENUM(SortKey)

    explicit DatasetFilterModel(DatasetListModel *source, QObject *parent = nullptr);

    QString search() const { return m_search; }
    void setSearch(const QString &search);
    int stateFilter() const { return m_stateFilter; } // -1: every state
    void setStateFilter(int state);
    bool filtered() const { return !m_search.isEmpty() || m_stateFilter >= 0; }
    SortKey sortKey() const { return m_sortKey; }
    bool descending() const { return m_descending; }
    int count() const { return rowCount(); }

    // a second click on the same column turns the order around
    Q_INVOKABLE void sortBy(DatasetFilterModel::SortKey key);
    Q_INVOKABLE void clearFilters();
    int sourceRow(int row) const;
    int rowOfPid(const QString &pid) const;
    bool lists(int sourceRow) const { return mapFromSource(m_source->index(sourceRow)).isValid(); }
    Q_INVOKABLE QString pidAt(int row) const;

#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
    // Looked up by QSortFilterProxyModel through the meta-object system (it cannot be virtual before Qt 7).
    // Without it every tick would make the proxy check the row's place in the order again.
    Q_INVOKABLE QSortFilterProxyModel::DataChangeRelevanceFlags dataChangeRelevanceFlags(const QModelIndex &sourceTopLeft,
                                                                                         const QModelIndex &sourceBottomRight,
                                                                                         const QList<int> &roles) const;
#endif

signals:
    void filterChanged();
    void sortChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    // QSortFilterProxyModel::invalidateFilter() is deprecated once begin/endFilterChange() both exist (Qt 6.10)
    template <typename Change>
    void changeFilter(Change change)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
        change();
        endFilterChange(Direction::Rows);
#else
        change();
        invalidateFilter();
#endif
    }

    // a filter change moves rows in many small runs: say "the count changed" once afterwards, not once per run
    void scheduleCountChanged();

    DatasetListModel *m_source;
    QString m_search;
    int m_stateFilter = -1;
    SortKey m_sortKey = ByCreated;
    bool m_descending = true;
    bool m_countPending = false;
};
