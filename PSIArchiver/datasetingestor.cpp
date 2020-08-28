#include "datasetingestor.h"
#include "messagehandlergui.h"

using namespace SciCat;

DatasetIngestor::DatasetIngestor(QWidget *parent) : QObject(parent){
    ingestJob = std::make_shared<ProcessHandler>(parent);

    connect(ingestJob.get(), SIGNAL(finished(int)), this, SLOT(ingestorFinished(int)));
    connect(ingestJob.get(), SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ingestorFailed(QProcess::ProcessError)));
    base = dynamic_cast<Dialog*>(parent);

}

void DatasetIngestor::prepareIngestion(){
    QSettings settingsContainer("PSI", "SciCat");

    QString dirpath = settingsContainer.value("logPath", base->defaultSettings.logPath).toString() + "/ingestion/" + base->scicat->experiment()->getDatasetName() + QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    m_ingestionLogDir = dirpath;
    if (!QDir(dirpath).exists()){
        qDebug()<<"Output directory does not exist";
        if (!QDir().mkpath(dirpath)){
            MessageHandlerGUI::sendWarning(base->ui->warningLabel, "Could not write to output directory.");
        }
    }

    QString metafname = QDir(dirpath).filePath(settingsContainer.value("filenameMetadata", base->defaultSettings.filenameMetadata).toString());
    QFile metadataFile(metafname);
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        MessageHandlerGUI::sendWarning(base->ui->warningLabel, "Could not open metadata file.");
        return;
    }
    m_metadataFilename = metafname;

    #ifdef WINDOWS_SPECIFIC_TOOLS
    for (auto& f: base->filelistingContainer){
        f = "/" + f.replace(":","");
    }
    #endif

    // prepare metadata file
    QJsonObject metadata;
    bool isRawData = (bool) base->ui->comboBoxDatatype->currentIndex();
    auto sourceFolder = extractSourceFolder(base->filelistingContainer);
    metadata["sourceFolder"] = sourceFolder;
    metadata["owner"] = base->scicat->experiment()->getOwner();
    metadata["ownerGroup"] = base->scicat->experiment()->getOwnerGroup();
    metadata["ownerEmail"] = base->scicat->experiment()->getOwnerEmail();
    metadata["description"] = base->scicat->experiment()->getDescription();
    metadata["datasetName"] = base->scicat->experiment()->getDatasetName();


    if (isRawData){
        qDebug() << "Preparing metadata file for raw data";
        metadata["principalInvestigator"] = base->scicat->experiment()->getPIemail();
        metadata["creationLocation"] = base->scicat->experiment()->getCreationLocation();
        metadata["type"] = "raw";
        auto res = base->ui->treeWidget->findItems("scientificMetadata", Qt::MatchFlag::MatchExactly);
        QJsonArray scMetadata;
        for (int ii=0; ii<res.size(); ii++){
            qDebug() << res[ii]->childCount();
            scMetadata.push_back(treeModel2Json(res[ii]));
        }
        metadata["scientificMetadata"] = scMetadata;
    } else {
        qDebug() << "Preparing metadata file for derived data";
        metadata["type"] = "derived";
        metadata["investigator"] = base->scicat->experiment()->getInvestigator();
        auto dsmodel = base->ui->tableWidget->model();
        QStringList inputDatasetsList;
        for (int ii=0; ii<dsmodel->rowCount(); ii++){
            inputDatasetsList.push_back(dsmodel->data(dsmodel->index(ii,1), Qt::DisplayRole).toString());
        }
        if (inputDatasetsList.length()==0){
            metadata["inputDatasets"] = QJsonArray::fromStringList(QStringList("N/A"));
        } else {
            metadata["inputDatasets"] = QJsonArray::fromStringList(inputDatasetsList);
        }
        if (base->scicat->experiment()->getUsedSoftware().length()==0){
            metadata["usedSoftware"] = QJsonArray::fromStringList(QStringList("N/A"));
        } else {
            metadata["usedSoftware"] = QJsonArray::fromStringList(QStringList(base->scicat->experiment()->getUsedSoftware()));
        }
    }


    QJsonDocument saveDoc(metadata);
    metadataFile.write(saveDoc.toJson());
    metadataFile.close();

    QString filefname = QDir(dirpath).filePath(settingsContainer.value("filenameFilelisting", base->defaultSettings.filenameFilelisting).toString());
    QFile filelistingFile(filefname);
    if (!filelistingFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        MessageHandlerGUI::sendWarning(base->ui->warningLabel, "Could not open filelisting file.");
        return;
    }
    m_filelistingFilename = filefname;

    QTextStream out(&filelistingFile);
    for (auto v:base->filelistingContainer){
        QString tmpFilename = v.remove(sourceFolder);
        if (tmpFilename[0]=="/"){
            tmpFilename.remove(0,1);
        }
        out << tmpFilename << "\n";
    }
    filelistingFile.close();
}

void DatasetIngestor::start(QTextEdit* outStream){
    QSettings settingsContainer("PSI", "SciCat");

    if (outStream != nullptr){
        ingestJob->setOutputStream(outStream);
    }

    // compile ingestion command
    QStringList arguments;

    if (base->ui->checkBoxIngest->isChecked()>0){
        arguments << "--ingest";
    }
    if (base->ui->checkBoxArchive->isChecked()>0){
        arguments << "--autoarchive";
    }
    switch (base->scicat->serverID){
    case 0:
        break;
    case 1:
        arguments << "-testenv";
        break;
    case 2:
        arguments << "-devenv";
        break;
    }

#ifdef WINDOWS_SPECIFIC_TOOLS
    arguments << "--noninteractive" << "--allowexistingsource" << "--user" << base->scicat->user()->getUsername() + ":" + base->scicat->user()->getPword() << m_metadataFilename << m_filelistingFilename;
#else
    arguments << "--noninteractive" << "--allowexistingsource" << "--token" << base->scicat->user()->getToken() << m_metadataFilename << m_filelistingFilename;
#endif

    // start ingestion
    qDebug() << "starting ingestion";

    if (settingsContainer.value("printIngestion", base->defaultSettings.printIngestion).toBool()){
        qDebug() << settingsContainer.value("datasetIngestorBinary", base->defaultSettings.datasetIngestorBinary).toString() << " " << arguments;
    }
    if (!settingsContainer.value("disableSystemCalls", base->defaultSettings.disableSystemCalls).toBool()){
        ingestJob->start(settingsContainer.value("datasetIngestorBinary", base->defaultSettings.datasetIngestorBinary).toString(), arguments);
    } else {
        ingestJob->finished(0);
    }

}

void DatasetIngestor::ingestorFinished(int val){
    qDebugLine("DatasetIngestor") << "ingestion finished";
    emit finished(val);
}

void DatasetIngestor::ingestorFailed(QProcess::ProcessError error){
    qDebugLine("DatasetIngestor") << "ingestion failed";
    emit errorOccurred(error);
}

QString DatasetIngestor::getIngestionLogDir() const
{
    return m_ingestionLogDir;
}

std::shared_ptr<ProcessHandler> DatasetIngestor::getIngestJob() const
{
    return ingestJob;
}
