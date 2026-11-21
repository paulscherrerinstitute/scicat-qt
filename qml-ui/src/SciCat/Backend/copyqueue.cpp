#include "copyqueue.h"

#include "clitool.h"
#include "session.h"
#include "settings.h"

#include <QDir>

using namespace scicat;

CopyTask::CopyTask(const QString &pid, const QString &name, const QString &folder, QObject *parent)
    : QObject(parent)
    , m_pid(pid)
    , m_name(name)
    , m_folder(folder)
    , m_createdAt(QDateTime::currentDateTime())
{
    connect(&m_process, &ProcessRunner::line, this, &CopyTask::append);
    connect(&m_process, &ProcessRunner::failedToStart, this, [this](const QString &reason) {
        append(tr("Could not start scicat-cli: %1").arg(reason));
        finish(Failed, tr("scicat-cli could not be started. Check its path in Settings."));
    });
    connect(&m_process, &ProcessRunner::finished, this, [this](int exitCode, bool crashed) {
        if (m_cancelRequested)
            finish(Cancelled, tr("Stopped. Files copied so far stay in the folder."));
        else if (crashed)
            finish(Failed, tr("scicat-cli stopped unexpectedly."));
        else if (exitCode != 0)
            finish(Failed, tr("The copy failed (exit code %1). The log says why.").arg(exitCode));
        else
            finish(Succeeded, tr("Copied to %1").arg(QDir::toNativeSeparators(m_folder)));
    });
}

void CopyTask::start(const QString &program, const QStringList &arguments, const QString &shownCommand)
{
    if (!shownCommand.isEmpty())
        append(QStringLiteral("$ ") + shownCommand);
    m_state = Running;
    emit changed();
    m_process.start(program, arguments);
}

void CopyTask::cancel()
{
    if (!active())
        return;
    m_cancelRequested = true;
    if (m_state == Queued) {
        finish(Cancelled, tr("Removed from the queue before it started."));
        return;
    }
    m_process.cancel();
}

void CopyTask::append(const QString &line)
{
    m_log += line + QLatin1Char('\n');
    emit logChanged();
}

void CopyTask::finish(State state, const QString &resultText)
{
    m_state = state;
    m_resultText = resultText;
    emit changed();
    emit finished();
}

CopyQueue::CopyQueue(Session *session, Settings *settings, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_settings(settings)
{
}

int CopyQueue::activeCount() const
{
    int active = 0;
    for (const CopyTask *task : m_tasks)
        active += task->active();
    return active;
}

CopyTask *CopyQueue::enqueue(const QString &pid, const QString &name, const QString &folder)
{
    auto *task = new CopyTask(pid, name, folder, this);
    m_tasks.prepend(task);
    connect(task, &CopyTask::changed, this, &CopyQueue::tasksChanged);
    connect(task, &CopyTask::finished, this, &CopyQueue::startNext);
    emit taskAdded(task);
    emit tasksChanged();
    startNext();
    return task;
}

void CopyQueue::cancelAll()
{
    const QList<CopyTask *> tasks = m_tasks;
    for (CopyTask *task : tasks)
        task->cancel();
}

void CopyQueue::removeFinished()
{
    for (int i = int(m_tasks.size()) - 1; i >= 0; --i) {
        if (!m_tasks.at(i)->active())
            m_tasks.takeAt(i)->deleteLater();
    }
    emit tasksChanged();
}

void CopyQueue::forgetAll()
{
    const QList<CopyTask *> tasks = m_tasks;
    m_tasks.clear();
    for (CopyTask *task : tasks) {
        disconnect(task, nullptr, this, nullptr);
        if (task->active()) {
            task->cancel();
            connect(task, &CopyTask::finished, task, &QObject::deleteLater);
        }
        if (!task->active())
            task->deleteLater();
    }
    emit tasksChanged();
}

void CopyQueue::startNext()
{
    int running = 0;
    for (const CopyTask *task : std::as_const(m_tasks))
        running += task->state() == CopyTask::Running;
    // oldest queued task first: the list is newest first
    for (int i = int(m_tasks.size()) - 1; i >= 0 && running < maxParallel; --i) {
        CopyTask *task = m_tasks.at(i);
        if (task->state() != CopyTask::Queued)
            continue;
        const QStringList arguments = retrieveArguments(m_session->serverId(), task->pid(), m_session->token(), task->folder());
        const QString shown = m_settings->showCommand() ? displayCommand(m_settings->cliPath(), arguments) : QString();
        task->start(m_settings->cliPath(), arguments, shown);
        ++running;
    }
}
