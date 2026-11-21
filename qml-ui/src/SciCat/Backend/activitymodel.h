#pragma once

#include "catalog.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class CopyQueue;
class CopyTask;
class IngestRun;
class RetrieveController;

// Everything that takes time, in one list: ingestions and copies started here, and the retrieve requests known to SciCat.
class ActivityModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.activity")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int runningCount READ runningCount NOTIFY runningCountChanged)
    Q_PROPERTY(int finishedLocalCount READ finishedLocalCount NOTIFY countChanged)

public:
    enum Kind { Ingest, Copy, Request };
    Q_ENUM(Kind)
    enum State { Waiting, Running, Done, Failed, Cancelled };
    Q_ENUM(State)
    enum Roles { KindRole = Qt::UserRole + 1, TitleRole, DetailRole, StateRole, StateTextRole, TimeTextRole, LogRole,
                 CanCancelRole, FolderRole, LocalRole, IdRole };

    ActivityModel(CopyQueue *copies, RetrieveController *retrieve, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_rows.size()); }
    int runningCount() const;
    int finishedLocalCount() const;
    void addIngest(IngestRun *run);
    bool lists(const IngestRun *run) const;

    Q_INVOKABLE void cancel(int row);
    // removes the finished ingestions and copies of this session from the list
    Q_INVOKABLE void clearFinished();
    void stopAll();
    void forgetSession();

signals:
    void countChanged();
    void runningCountChanged();

private:
    struct Row
    {
        Kind kind = Request;
        QPointer<IngestRun> ingest;
        QPointer<CopyTask> copy;
        scicat::RetrieveJob job;
    };
    void rebuild();
    void rowChanged(const QObject *object, const QList<int> &roles);
    State stateOf(const Row &row) const;
    static QString idOf(const Row &row);

    CopyQueue *m_copies;
    RetrieveController *m_retrieve;
    QList<QPointer<IngestRun>> m_ingests;
    QList<Row> m_rows;
};
