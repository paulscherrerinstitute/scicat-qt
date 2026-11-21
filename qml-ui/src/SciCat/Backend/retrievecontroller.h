#pragma once

#include "apiclient.h"
#include "catalog.h"
#include "datasetdetails.h"
#include "datasetlistmodel.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class CopyQueue;
class Session;
class Settings;
class Workspace;

// Getting data back: the datasets of the pgroup, requesting them from tape and copying retrieved ones to disk.
class RetrieveController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.retrieve")

    Q_PROPERTY(DatasetFilterModel *datasets READ datasets CONSTANT)
    Q_PROPERTY(DatasetDetails *details READ details CONSTANT)
    Q_PROPERTY(LoadState loadState READ loadState NOTIFY loadStateChanged)
    Q_PROPERTY(int loadedCount READ loadedCount NOTIFY loadedCountChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY loadStateChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY loadStateChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(QList<int> stateCounts READ stateCounts NOTIFY countsChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(int requestableCount READ requestableCount NOTIFY selectionChanged)
    Q_PROPERTY(int copyableCount READ copyableCount NOTIFY selectionChanged)
    Q_PROPERTY(int filteredSelection READ filteredSelection NOTIFY filteredSelectionChanged)
    Q_PROPERTY(QString selectionHint READ selectionHint NOTIFY selectionChanged)
    Q_PROPERTY(bool requesting READ requesting NOTIFY requestingChanged)
    Q_PROPERTY(QString noticeText READ noticeText NOTIFY noticeChanged)
    Q_PROPERTY(QString noticeTone READ noticeTone NOTIFY noticeChanged)
    Q_PROPERTY(QString currentPid READ currentPid WRITE setCurrentPid NOTIFY currentPidChanged)
    Q_PROPERTY(int currentRow READ currentRow NOTIFY currentRowChanged)
    Q_PROPERTY(bool copySupported READ copySupported CONSTANT)
    Q_PROPERTY(bool checkingTicket READ checkingTicket NOTIFY checkingTicketChanged)

public:
    enum LoadState { Idle, Loading, Loaded, Failed };
    Q_ENUM(LoadState)

    RetrieveController(scicat::ApiClient *api, Session *session, Workspace *workspace, Settings *settings, CopyQueue *copies,
                       QObject *parent = nullptr);

    DatasetFilterModel *datasets() const { return m_filter; }
    DatasetDetails *details() const { return m_details; }
    LoadState loadState() const { return m_loadState; }
    int loadedCount() const { return m_loadedCount; }
    QString errorText() const { return m_errorText; }
    QStringList warnings() const;
    int totalCount() const { return m_source->rowCount(); }
    QList<int> stateCounts() const;
    int selectedCount() const { return m_source->selectedCount(); }
    int requestableCount() const;
    int copyableCount() const;
    int filteredSelection() const; // 0 none, 1 some, 2 all of the listed datasets
    QString selectionHint() const;
    bool requesting() const { return m_requesting; }
    QString noticeText() const { return m_noticeText; }
    QString noticeTone() const { return m_noticeTone; }
    // The dataset the details panel shows. It is kept as an ID: rows move when the list is sorted, filtered or
    // loaded again, and currentRow says where that dataset is listed now (-1: not listed).
    QString currentPid() const { return m_currentPid; }
    void setCurrentPid(const QString &pid);
    int currentRow() const { return m_filter->rowOfPid(m_currentPid); }
    bool copySupported() const;
    bool checkingTicket() const { return m_ticketCheck != nullptr; }
    const QList<scicat::RetrieveJob> &jobs() const { return m_jobs; }
    QString datasetName(const QString &pid) const;

    // loads once per pgroup; reload() always asks SciCat again
    Q_INVOKABLE void ensureLoaded();
    Q_INVOKABLE void reload();
    // only the user's retrieve requests, which do not depend on the pgroup
    Q_INVOKABLE void refreshJobs();
    // the same, unless SciCat has been asked for them since signing in
    Q_INVOKABLE void ensureJobsLoaded();
    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void setListedSelected(bool selected);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void requestSelected();
    // answers through ticketChecked(): copying needs a Kerberos ticket
    Q_INVOKABLE void checkTicket();
    Q_INVOKABLE void copySelectedTo(const QUrl &folder);
    // says why "Copy" does nothing for the current selection
    Q_INVOKABLE void explainNothingToCopy();
    Q_INVOKABLE void dismissNotice();
    Q_INVOKABLE QString stateText(int state) const { return DatasetListModel::stateText(DatasetListModel::State(state)); }

signals:
    void loadStateChanged();
    void loadedCountChanged();
    void countsChanged();
    void selectionChanged();
    void filteredSelectionChanged();
    void currentRowChanged();
    void requestingChanged();
    void noticeChanged();
    void currentPidChanged();
    void checkingTicketChanged();
    void ticketChecked(bool hasTicket);
    void jobsChanged();

private:
    void forgetGroup();
    void forgetUser();
    void loadJobs(bool thenDatasets);
    void loadDatasets();
    void applyJobs();
    void updatePolling();
    void showCurrentDetails();
    void stopTicketCheck();
    void setNotice(const QString &tone, const QString &text);
    bool requestable(int sourceRow) const;

    scicat::ApiClient *m_api;
    Session *m_session;
    Workspace *m_workspace;
    Settings *m_settings;
    CopyQueue *m_copies;
    DatasetListModel *m_source;
    DatasetFilterModel *m_filter;
    DatasetDetails *m_details;
    LoadState m_loadState = Idle;
    QString m_loadedGroup;
    int m_loadedCount = 0;
    QString m_errorText;
    QStringList m_warnings;
    QString m_jobsWarning;
    QString m_shownJobsWarning;
    QList<scicat::RetrieveJob> m_jobs;
    bool m_jobsAsked = false;
    QPointer<scicat::PagedFetch> m_jobsFetch;
    QPointer<scicat::PagedFetch> m_datasetsFetch;
    QPointer<scicat::ApiReply> m_requestReply;
    bool m_requesting = false;
    QString m_noticeText;
    QString m_noticeTone;
    QString m_currentPid;
    QTimer m_poll;
    QTimer m_detailsDelay;
    QElapsedTimer m_sinceCurrentChange;
    qint64 m_lastRequestAt = 0;
    QProcess *m_ticketCheck = nullptr;
};
