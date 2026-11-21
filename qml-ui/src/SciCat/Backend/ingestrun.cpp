#include "ingestrun.h"

#include "appsettings.h"
#include "clitool.h"
#include "fileentrymodel.h"
#include "formatting.h"
#include "settings.h"

#include <QDir>
#include <QFile>

using namespace scicat;

IngestRun::IngestRun(const Request &request, FileEntryModel *files, Settings *settings, QObject *parent)
    : QObject(parent)
    , m_request(request)
    , m_files(files)
    , m_settings(settings)
    , m_startedAt(QDateTime::currentDateTime())
{
    m_clock.setInterval(1000);
    connect(&m_clock, &QTimer::timeout, this, &IngestRun::elapsedChanged);
    connect(&m_process, &ProcessRunner::line, this, &IngestRun::append);
    connect(&m_process, &ProcessRunner::failedToStart, this, [this](const QString &reason) {
        append(tr("Could not start %1: %2").arg(m_settings->cliPath(), reason));
        finish(Failed, tr("scicat-cli could not be started. Check its path in Settings."));
    });
    connect(&m_process, &ProcessRunner::finished, this, [this](int exitCode, bool crashed) {
        if (m_cancelRequested)
            finish(Cancelled, tr("Stopped. Nothing more was sent."));
        else if (crashed)
            finish(Failed, tr("scicat-cli stopped unexpectedly."));
        else if (exitCode != 0)
            finish(Failed, tr("scicat-cli reported a problem (exit code %1). The log below says what it was.").arg(exitCode));
        else if (dryRun())
            finish(Succeeded, tr("The check passed. Nothing was registered."));
        else if (autoArchive())
            finish(Succeeded, tr("The dataset is registered and its archiving is requested."));
        else
            finish(Succeeded, tr("The dataset is registered in the catalogue."));
    });
}

int IngestRun::elapsedSeconds() const
{
    return int(m_startedAt.secsTo(m_endedAt.isValid() ? m_endedAt : QDateTime::currentDateTime()));
}

void IngestRun::start()
{
    m_clock.start();
    if (m_files->scanning()) {
        append(tr("Counting the files…"));
        connect(m_files, &FileEntryModel::totalsChanged, this, [this] {
            if (m_state == Preparing && !m_files->scanning()) {
                disconnect(m_files, &FileEntryModel::totalsChanged, this, nullptr);
                prepare();
            }
        });
        return;
    }
    prepare();
}

void IngestRun::cancel()
{
    if (!active())
        return;
    m_cancelRequested = true;
    if (m_state == Preparing) {
        disconnect(m_files, &FileEntryModel::totalsChanged, this, nullptr);
        finish(Cancelled, tr("Stopped before anything was sent."));
        return;
    }
    append(tr("Stopping…"));
    m_process.cancel();
}

QString IngestRun::saveLogTo(const QUrl &fileUrl) const
{
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return tr("Could not write %1: %2").arg(file.fileName(), file.errorString());
    file.write(m_log.toUtf8());
    return QString();
}

void IngestRun::prepare()
{
    if (m_files->overLimit()) {
        finish(Failed, tr("The selection holds %1 files. A dataset can hold fewer than %2: remove some entries and submit them as a second dataset.")
                           .arg(formatCount(m_files->totalFiles()), formatCount(AppSettings::maxFilesPerDataset)));
        return;
    }
    const IngestFiles files = writeIngestFiles(m_request.draft, m_settings->logPath(), m_settings->metadataFileName(),
                                               m_settings->fileListingFileName(), QDateTime::currentSecsSinceEpoch());
    m_logFolder = files.directory;
    if (!files.ok()) {
        append(files.error);
        finish(Failed, tr("The files for scicat-cli could not be written. %1").arg(files.error));
        return;
    }
    launch(files);
}

void IngestRun::launch(const IngestFiles &files)
{
    const QStringList arguments = ingestArguments(m_request.ingest, m_request.autoArchive, m_request.serverId, m_request.token,
                                                  files.metadataFile, files.fileListingFile);
    if (m_settings->showCommand())
        append(QStringLiteral("$ ") + displayCommand(m_settings->cliPath(), arguments));
    m_request.token.clear(); // handed to scicat-cli just now; a retry is a new run with the session's token
    m_state = Running;
    emit stateChanged();
    if (m_settings->disableSystemCalls()) {
        append(tr("System calls are switched off in Settings: scicat-cli was not run."));
        finish(Succeeded, tr("Nothing was run, because system calls are switched off in Settings."));
        return;
    }
    m_process.start(m_settings->cliPath(), arguments);
}

void IngestRun::append(const QString &line)
{
    m_log += line + QLatin1Char('\n');
    emit lineAdded(line);
    emit logChanged();
}

void IngestRun::finish(State state, const QString &resultText)
{
    m_clock.stop();
    m_endedAt = QDateTime::currentDateTime();
    m_resultText = resultText;
    m_state = state;

    // as in the Widgets GUI: either the folder with the CLI's input files goes away, or the output is kept next to them
    if (!m_logFolder.isEmpty() && QDir(m_logFolder).exists()) {
        if (m_settings->deleteLog()) {
            if (!QDir(m_logFolder).removeRecursively())
                append(tr("The folder %1 could not be removed.").arg(QDir::toNativeSeparators(m_logFolder)));
            else
                m_logFolder.clear();
        } else if (m_settings->saveLog()) {
            const QString problem = saveLogTo(QUrl::fromLocalFile(m_logFolder + QStringLiteral("/ingestion.log")));
            if (!problem.isEmpty())
                append(problem);
        }
    }
    emit elapsedChanged();
    emit stateChanged();
    emit finished();
}
