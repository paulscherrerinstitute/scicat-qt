#include "datasetretriever.h"
#include "QDebug"
#include "datacatalogue.h"
#include "messagehandlergui.h"

using namespace SciCat;

DatasetRetriever::DatasetRetriever(QWidget *parent)
{
    retrieverJob = std::make_shared<ProcessHandler>(parent);

    #ifdef NO_DATACATALOGUE_TOOLS
    checkSumProcess = std::make_shared<ProcessHandler>(parent);
    ProcessHandler::connect(retrieverJob.get(), SIGNAL(finished()), this, SLOT(checkSumVerification()));
    ProcessHandler::connect(checkSumProcess->getJob(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(checkSumVerificationFinished(int)));
#endif
    base = dynamic_cast<Dialog*>(parent);

}

void DatasetRetriever::start(const QString &pid, const QString &outdir){
QSettings settingsContainer("PSI", "SciCat");
#ifdef NO_DATACATALOGUE_TOOLS
    // communication without using the datasetRetriever
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
#else
    // use the scicat-cli datasetRetriever command to communicate with the retrieve server
    QStringList arguments{"datasetRetriever"};
    arguments << DataCatalogue::cliEnvironmentArguments(base->scicat->serverID);

    arguments << "--dataset" << pid << "--token" << base->scicat->user()->getToken() << "--retrieve" << outdir;
    retrieverJob->start(settingsContainer.value("scicatCliBinary", base->defaultSettings.scicatCliBinary).toString(), arguments);

#endif
}

QProcess *DatasetRetriever::getRetrieverJob() const
{
    return retrieverJob->getJob();
}

void DatasetRetriever::checkSumVerificationFinished(int val){
    qDebugLine("checkSumVerification") << "Finished";
    if (val!=0){
        MessageHandlerGUI::sendWarning(base->ui->warningLabel, "Retrieval finished but CheckSumVerification failed. Data might be corrupted.");
    }
}

void DatasetRetriever::checkSumVerification() {
    auto argumentString = "cd " + m_outdir + " ; sed '/is_directory$/d' __checksum_filename_*__ |  awk -v FS='    ' '/^[^#]/{print $2,$1}' | sha1sum -c";
    QStringList arguments;
    arguments << "-c" << argumentString;
    qDebugLine("checkSumVerification") << arguments;
    checkSumProcess->start("bash", arguments);
}



