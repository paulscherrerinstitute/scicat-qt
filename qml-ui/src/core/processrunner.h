#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace scicat {

// One external command whose merged output arrives line by line. Replaces ProcessHandler of the Widgets GUI.
class ProcessRunner : public QObject
{
    Q_OBJECT
public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    void start(const QString &program, const QStringList &arguments);
    // asks the process to end, kills it if it does not within two seconds
    void cancel();
    bool running() const { return m_process.state() != QProcess::NotRunning; }
    qint64 processId() const { return m_process.processId(); }

signals:
    void line(const QString &text);
    void failedToStart(const QString &reason);
    void finished(int exitCode, bool crashed);

private:
    void readOutput(bool flush);

    QProcess m_process;
    QByteArray m_partialLine;
    bool m_reported = false;
};

} // namespace scicat
