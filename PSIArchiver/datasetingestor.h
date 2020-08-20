#ifndef DATASETINGESTOR_H
#define DATASETINGESTOR_H

#include <QWidget>
#include <QProcess>
#include "dialog.h"
#include "processhandler.h"

namespace SciCat {
class DatasetIngestor : public QObject
{
    Q_OBJECT
public:
    explicit DatasetIngestor(QWidget *parent = nullptr);
    void prepareIngestion();
    void start(QTextEdit* outStream = nullptr);
    std::shared_ptr<ProcessHandler> getIngestJob() const;

    QString getIngestionLogDir() const;

signals:
    void finished(int);
    void errorOccurred(QProcess::ProcessError error);
private slots:
    void ingestorFinished(int);
    void ingestorFailed(QProcess::ProcessError error);
private:
    std::shared_ptr<ProcessHandler> ingestJob;
    Dialog *base;
    QString m_ingestionLogDir;
    QString m_metadataFilename;
    QString m_filelistingFilename;

};
}


#endif // DATASETINGESTOR_H
