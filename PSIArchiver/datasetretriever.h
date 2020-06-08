#ifndef DATASETRETRIEVER_H
#define DATASETRETRIEVER_H

#include <QWidget>
#include <QProcess>
#include "dialog.h"
#include "processhandler.h"

class DatasetRetriever : public QObject
{
    Q_OBJECT
public:
    DatasetRetriever(QWidget *parent = nullptr);
    void start(const QString &pid, const QString &outdir);
    QProcess *getRetrieverJob() const;

private:
    std::shared_ptr<ProcessHandler> retrieverJob;
    std::shared_ptr<ProcessHandler> checkSumProcess;
    Dialog *base;
    QString m_outdir;
    QString m_pid;
private slots:
    void checkSumVerification();
    void checkSumVerificationFinished(int val);
};

#endif // DATASETRETRIEVER_H
