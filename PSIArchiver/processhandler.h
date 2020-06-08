#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <QProcess>
#include <QWidget>

class ProcessHandler : public QObject
{
    Q_OBJECT
public:
    ProcessHandler(QWidget *parent=nullptr);
    ~ProcessHandler();
    QProcess *getJob() const;
    void start(QString cmd, QStringList arguments);
private slots:
    void readOutput();
    void processFinished(int);
    void processFailed(QProcess::ProcessError);
signals:
    void finished();
private:
    QProcess* job;
};

#endif // PROCESSHANDLER_H
