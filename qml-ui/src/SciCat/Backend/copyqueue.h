#pragma once

#include "processrunner.h"

#include <QDateTime>
#include <QList>
#include <QObject>

class Session;
class Settings;

// One "scicat-cli datasetRetriever" call that copies a retrieved dataset from the retrieve server to a local folder.
class CopyTask : public QObject
{
    Q_OBJECT
public:
    enum State { Queued, Running, Succeeded, Failed, Cancelled };

    CopyTask(const QString &pid, const QString &name, const QString &folder, QObject *parent = nullptr);

    QString pid() const { return m_pid; }
    QString name() const { return m_name; }
    QString folder() const { return m_folder; }
    State state() const { return m_state; }
    bool active() const { return m_state == Queued || m_state == Running; }
    QString log() const { return m_log; }
    QString resultText() const { return m_resultText; }
    QDateTime createdAt() const { return m_createdAt; }

    void start(const QString &program, const QStringList &arguments, const QString &shownCommand);
    void cancel();

signals:
    void changed();
    void logChanged();
    void finished();

private:
    void append(const QString &line);
    void finish(State state, const QString &resultText);

    QString m_pid;
    QString m_name;
    QString m_folder;
    State m_state = Queued;
    QString m_log;
    QString m_resultText;
    QDateTime m_createdAt;
    scicat::ProcessRunner m_process;
    bool m_cancelRequested = false;
};

// Runs the copies two at a time, so selecting forty datasets does not start forty transfers at once.
class CopyQueue : public QObject
{
    Q_OBJECT
public:
    static constexpr int maxParallel = 2;

    CopyQueue(Session *session, Settings *settings, QObject *parent = nullptr);

    const QList<CopyTask *> &tasks() const { return m_tasks; }
    int activeCount() const;
    CopyTask *enqueue(const QString &pid, const QString &name, const QString &folder);
    void cancelAll();
    void removeFinished();
    // empties the list; a copy that is still stopping deletes itself once its process has ended
    void forgetAll();

signals:
    void taskAdded(CopyTask *task);
    void tasksChanged();

private:
    void startNext();

    Session *m_session;
    Settings *m_settings;
    QList<CopyTask *> m_tasks;
};
