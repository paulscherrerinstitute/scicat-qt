#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <QProcess>
#include <QWidget>
#include <QTextEdit>

class ProcessHandler : public QObject
{
    Q_OBJECT
public:
    ProcessHandler(QWidget *parent=nullptr);
    ~ProcessHandler();
    QProcess *getJob() const;
    void start(QString cmd, QStringList arguments);
    void setOutputStream(QTextEdit* out);

private slots:
    void readOutput();
    void processFinished(int);
    void processFailed(QProcess::ProcessError);
signals:
    void finished(int);
private:
    QProcess* job;
    QTextEdit* m_outStream=nullptr;
};

#endif // PROCESSHANDLER_H
