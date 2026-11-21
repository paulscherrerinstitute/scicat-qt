#include "backend.h"

#include "formatting.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

namespace {
Backend *s_instance = nullptr;
}

Backend::Backend(const BackendConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_store(config.settingsFile)
{
    m_api.setApiOverride(config.apiOverride);
    m_settings = new Settings(&m_store, this);
    m_session = new Session(&m_api, m_settings, config.applicationDir, this);
    m_workspace = new Workspace(&m_api, m_session, m_settings, this);
    m_copies = new CopyQueue(m_session, m_settings, this);
    m_archive = new ArchiveDraft(&m_api, m_session, m_workspace, m_settings, this);
    m_retrieve = new RetrieveController(&m_api, m_session, m_workspace, m_settings, m_copies, this);
    m_activity = new ActivityModel(m_copies, m_retrieve, this);
    connect(m_archive, &ArchiveDraft::runStarted, m_activity, &ActivityModel::addIngest);
    // Signing out, in this order: stop what runs, then drop everything that belonged to the user. Done here and not
    // by each object for itself, because the order matters: a draft whose run is still stopping must go as well.
    connect(m_session, &Session::aboutToSignOut, this, [this] {
        m_activity->stopAll();
        m_archive->forgetSession();
        m_activity->forgetSession();
    });
    // a run is deleted once it has ended and neither the form nor the activity list shows it any more
    connect(m_archive, &ArchiveDraft::runStarted, this, [this](IngestRun *run) {
        connect(run, &IngestRun::finished, this, &Backend::collectRuns, Qt::QueuedConnection);
    });
    connect(m_archive, &ArchiveDraft::runChanged, this, &Backend::collectRuns, Qt::QueuedConnection);
    connect(m_activity, &ActivityModel::countChanged, this, &Backend::collectRuns, Qt::QueuedConnection);
}

Backend::~Backend()
{
    if (s_instance == this)
        s_instance = nullptr;
}

Backend *Backend::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT(s_instance);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void Backend::setInstance(Backend *backend)
{
    s_instance = backend;
}

BackendConfig Backend::configFromEnvironment(const QString &applicationDir, const QString &version)
{
    BackendConfig config;
    config.applicationDir = applicationDir;
    config.version = version;
    config.settingsFile = qEnvironmentVariable("SCICAT_QML_SETTINGS");
    config.apiOverride = qEnvironmentVariable("SCICAT_QML_API");
    config.windowTag = qEnvironmentVariable("SCICAT_WINDOW_TAG");
    config.startHidden = qEnvironmentVariableIsSet("SCICAT_QML_HIDDEN");
    return config;
}

void Backend::copyToClipboard(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}

void Backend::openUrl(const QUrl &url) const
{
    QDesktopServices::openUrl(url);
}

void Backend::showFolder(const QString &path) const
{
    const QFileInfo info(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? info.absoluteFilePath() : info.absolutePath()));
}

QString Backend::formatCount(int count) const
{
    return scicat::formatCount(count);
}

QString Backend::formatDuration(int seconds) const
{
    return scicat::formatDuration(seconds);
}

void Backend::stopAll()
{
    m_activity->stopAll();
}

void Backend::collectRuns()
{
    const QList<IngestRun *> runs = m_archive->findChildren<IngestRun *>(Qt::FindDirectChildrenOnly);
    for (IngestRun *run : runs) {
        if (!run->active() && run != m_archive->run() && !m_activity->lists(run))
            run->deleteLater();
    }
}
