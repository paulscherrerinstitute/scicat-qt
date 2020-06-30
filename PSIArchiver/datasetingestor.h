#ifndef DATASETINGESTOR_H
#define DATASETINGESTOR_H

#include <QWidget>
#include <QProcess>
#include "dialog.h"
#include "processhandler.h"

class DatasetIngestor : public QObject
{
    Q_OBJECT
public:
    explicit DatasetIngestor(QWidget *parent = nullptr);
    void prepareIngestion();
    void start();
private:
    std::shared_ptr<ProcessHandler> ingestJob;
    Dialog *base;
    QString m_ingestionLogDir;
    QString m_metadataFilename;
    QString m_filelistingFilename;

};

#endif // DATASETINGESTOR_H
