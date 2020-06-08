#include "datasetretriever.h"
#include "QDebug"
#include "datacatalogueClasses.h"

DatasetRetriever::DatasetRetriever(QWidget *parent)
{
    retrieverJob = std::make_shared<ProcessHandler>(parent);
    checkSumProcess = std::make_shared<ProcessHandler>(parent);
    ProcessHandler::connect(retrieverJob.get(), SIGNAL(finished()), this, SLOT(checkSumVerification()));
    ProcessHandler::connect(checkSumProcess->getJob(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(checkSumVerificationFinished(int)));

    base = dynamic_cast<Dialog*>(parent);

}

void DatasetRetriever::start(const QString &pid, const QString &outdir){

#ifndef WINDOWS_SPECIFIC_TOOLS
    // LINUX + MacOS
    const QString rsyncPath = "rsync";
    const QString sshPath = "ssh";
#else
    // Windows
    const QString rsyncPath = "rsync.exe";
    const QString sshPath = "ssh.exe";
#endif


    QStringList pidSplit = pid.split("/");
    Q_ASSERT(pidSplit.length()==2);

    m_pid = pid;
    m_outdir = QDir(outdir).filePath(pidSplit[1]);

    QStringList arguments;
    arguments << "-av" << "-e" << sshPath + " -o GSSAPIAuthentication=yes -o StrictHostKeyChecking=no" << base->scicat->user()->getUsername() +"@" + SciCat::DataCatalogue::serverListRetrieve()[base->scicat->serverID] + ":retrieve/" + pidSplit[1] << outdir;
    qDebug() << arguments;
    retrieverJob->start(rsyncPath, arguments);
}

QProcess *DatasetRetriever::getRetrieverJob() const
{
    return retrieverJob->getJob();
}

void DatasetRetriever::checkSumVerificationFinished(int val){
    qDebugLine("checkSumVerification") << "Finished";
    if (val!=0){
        base->sendWarning("Retrieval finished but CheckSumVerification failed. Data might be corrupted.");
    }
}

void DatasetRetriever::checkSumVerification() {
    auto argumentString = "cd " + m_outdir + " ; sed '/is_directory$/d' __checksum_filename_*__ |  awk -v FS='    ' '/^[^#]/{print $2,$1}' | sha1sum -c";
    QStringList arguments;
    arguments << "-c" << argumentString;
    qDebugLine("checkSumVerification") << arguments;
    checkSumProcess->start("bash", arguments);
}



