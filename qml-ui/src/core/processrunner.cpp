#include "processrunner.h"

#include <QTimer>

namespace scicat {

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] { readOutput(false); });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || m_reported)
            return;
        m_reported = true;
        emit failedToStart(m_process.errorString());
    });
    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        readOutput(true);
        if (m_reported)
            return;
        m_reported = true;
        emit finished(exitCode, status == QProcess::CrashExit);
    });
}

ProcessRunner::~ProcessRunner()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.disconnect(this);
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void ProcessRunner::start(const QString &program, const QStringList &arguments)
{
    m_reported = false;
    m_partialLine.clear();
    m_process.start(program, arguments);
}

void ProcessRunner::cancel()
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_process.terminate();
    QTimer::singleShot(2000, &m_process, [this] {
        if (m_process.state() != QProcess::NotRunning)
            m_process.kill();
    });
}

void ProcessRunner::readOutput(bool flush)
{
    m_partialLine += m_process.readAllStandardOutput();
    int newline;
    while ((newline = m_partialLine.indexOf('\n')) >= 0) {
        QByteArray text = m_partialLine.left(newline);
        m_partialLine.remove(0, newline + 1);
        if (text.endsWith('\r'))
            text.chop(1);
        emit line(QString::fromUtf8(text));
    }
    if (flush && !m_partialLine.isEmpty()) {
        emit line(QString::fromUtf8(m_partialLine));
        m_partialLine.clear();
    }
}

} // namespace scicat
