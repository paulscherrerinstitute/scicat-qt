#ifndef DATASETRETRIEVER_H
#define DATASETRETRIEVER_H

#include <QWidget>
#include <QProcess>

class DatasetRetriever : public QObject
{
    Q_OBJECT
public:
    DatasetRetriever(QWidget *parent = nullptr);
private slots:
    void readRetrieverOutput();
    void retrieverFinished(int);
    void retrieverFailed(QProcess::ProcessError);
private:
    QProcess* retriever;

};

#endif // DATASETRETRIEVER_H
