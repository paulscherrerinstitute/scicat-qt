#pragma once

#include "activitymodel.h"
#include "apiclient.h"
#include "appsettings.h"
#include "archivedraft.h"
#include "copyqueue.h"
#include "retrievecontroller.h"
#include "session.h"
#include "settings.h"
#include "workspace.h"

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

struct BackendConfig
{
    QString settingsFile;   // empty: the native store of PSI/SciCatQml
    QString apiOverride;    // local trials: the API base every server is replaced with
    QString applicationDir; // where a bundled scicat-cli is looked for
    QString windowTag;      // appended to the window title, e.g. "worktree@commit"
    QString version;
    bool startHidden = false; // automated grabs: the window is never shown
};

// The one object QML talks to. It owns the application state and hands out its parts.
class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Settings *settings READ settings CONSTANT)
    Q_PROPERTY(Session *session READ session CONSTANT)
    Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)
    Q_PROPERTY(ArchiveDraft *archive READ archive CONSTANT)
    Q_PROPERTY(RetrieveController *retrieve READ retrieve CONSTANT)
    Q_PROPERTY(ActivityModel *activity READ activity CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString windowTag READ windowTag CONSTANT)
    Q_PROPERTY(bool startHidden READ startHidden CONSTANT)

public:
    explicit Backend(const BackendConfig &config, QObject *parent = nullptr);
    ~Backend() override;

    // QML gets the instance made in main(); the engine never constructs or deletes it
    static Backend *create(QQmlEngine *, QJSEngine *);
    static void setInstance(Backend *backend);
    static BackendConfig configFromEnvironment(const QString &applicationDir, const QString &version);

    Settings *settings() const { return m_settings; }
    Session *session() const { return m_session; }
    Workspace *workspace() const { return m_workspace; }
    ArchiveDraft *archive() const { return m_archive; }
    RetrieveController *retrieve() const { return m_retrieve; }
    ActivityModel *activity() const { return m_activity; }
    CopyQueue *copies() const { return m_copies; }
    scicat::ApiClient *api() { return &m_api; }
    QString version() const { return m_config.version; }
    QString windowTag() const { return m_config.windowTag; }
    bool startHidden() const { return m_config.startHidden; }

    Q_INVOKABLE void copyToClipboard(const QString &text) const;
    Q_INVOKABLE void openUrl(const QUrl &url) const;
    Q_INVOKABLE void showFolder(const QString &path) const;
    Q_INVOKABLE QUrl folderUrl(const QString &path) const { return QUrl::fromLocalFile(path); }
    Q_INVOKABLE QString localPath(const QUrl &url) const { return url.toLocalFile(); }
    Q_INVOKABLE QString formatCount(int count) const;
    Q_INVOKABLE QString formatDuration(int seconds) const;
    // stops every ingestion and copy started here; used before quitting and signing out
    Q_INVOKABLE void stopAll();

private:
    void collectRuns();

    BackendConfig m_config;
    scicat::AppSettings m_store;
    scicat::ApiClient m_api;
    Settings *m_settings;
    Session *m_session;
    Workspace *m_workspace;
    CopyQueue *m_copies;
    ArchiveDraft *m_archive;
    RetrieveController *m_retrieve;
    ActivityModel *m_activity;
};
