#pragma once

#include "datasetmetadata.h"
#include "processrunner.h"

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class FileEntryModel;
class Settings;

// One run of scicat-cli datasetIngestor, from writing its input files to the saved log.
class IngestRun : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by Backend.archive.submit()")

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(QString datasetName READ datasetName CONSTANT)
    Q_PROPERTY(bool dryRun READ dryRun CONSTANT)
    Q_PROPERTY(bool autoArchive READ autoArchive CONSTANT)
    Q_PROPERTY(QString serverLabel READ serverLabel CONSTANT)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY stateChanged)
    Q_PROPERTY(QString logFolder READ logFolder NOTIFY stateChanged)
    Q_PROPERTY(QDateTime startedAt READ startedAt CONSTANT)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedChanged)

public:
    enum State { Preparing, Running, Succeeded, Failed, Cancelled };
    Q_ENUM(State)

    struct Request
    {
        scicat::DatasetDraft draft;
        bool ingest = true;
        bool autoArchive = true;
        int serverId = 0;
        QString serverLabel;
        QString token;
    };

    IngestRun(const Request &request, FileEntryModel *files, Settings *settings, QObject *parent = nullptr);

    State state() const { return m_state; }
    bool active() const { return m_state == Preparing || m_state == Running; }
    QString datasetName() const { return m_request.draft.datasetName; }
    bool dryRun() const { return !m_request.ingest; }
    bool autoArchive() const { return m_request.autoArchive; }
    QString serverLabel() const { return m_request.serverLabel; }
    QString log() const { return m_log; }
    QString resultText() const { return m_resultText; }
    QString logFolder() const { return m_logFolder; }
    QDateTime startedAt() const { return m_startedAt; }
    int elapsedSeconds() const;

    void start();
    Q_INVOKABLE void cancel();
    // empty on success, else what went wrong
    Q_INVOKABLE QString saveLogTo(const QUrl &file) const;

signals:
    void stateChanged();
    void logChanged();
    void lineAdded(const QString &line);
    void elapsedChanged();
    void finished();

private:
    void prepare();
    void launch(const scicat::IngestFiles &files);
    void append(const QString &line);
    void finish(State state, const QString &resultText);

    Request m_request;
    FileEntryModel *m_files;
    Settings *m_settings;
    scicat::ProcessRunner m_process;
    State m_state = Preparing;
    QString m_log;
    QString m_resultText;
    QString m_logFolder;
    QDateTime m_startedAt;
    QDateTime m_endedAt;
    QTimer m_clock;
    bool m_cancelRequested = false;
};
