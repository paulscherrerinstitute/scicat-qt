#include "datasetretriever.h"
#include "QDebug"

DatasetRetriever::DatasetRetriever(QWidget *parent)
{
    retriever = new QProcess(parent);

    retriever->setProcessChannelMode(QProcess::MergedChannels);
    connect(retriever, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(retrieverFailed(QProcess::ProcessError)));
    connect(retriever, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(retrieverFinished(int)));
    connect(retriever, SIGNAL(readyReadStandardOutput()), this, SLOT(readRetrieverOutput()));

#ifndef WINDOWS_SPECIFIC_TOOLS
    QStringList arguments;
    arguments << "-av" << "-e" << "ssh -o GSSAPIAuthentication=yes -o StrictHostKeyChecking=no" << "wakonig_k@pbt-retrieve.psi.ch:retrieve/dae43524-ad3f-400d-9afb-ee91408931f5" << "./";
    qDebug() << arguments;
    retriever->start("rsync", arguments);
#else
    QStringList arguments;
    arguments << "-av" << "-e" << "ssh -o GSSAPIAuthentication=yes -o StrictHostKeyChecking=no" << "wakonig_k@pbt-retrieve.psi.ch:retrieve/dae43524-ad3f-400d-9afb-ee91408931f5" << "./";
    qDebug() << arguments;
    retriever->start("rsync.exe", arguments); // to be fixed
#endif

}

void DatasetRetriever::readRetrieverOutput(){
    qDebug() << "reading output";
    while(retriever->canReadLine()){
        auto tmp = retriever->readLine();
        qDebug() << tmp;
    }
}

void DatasetRetriever::retrieverFinished(int val){

    qDebug() << "Authentication finished with exit code " << val;

}

void DatasetRetriever::retrieverFailed(QProcess::ProcessError error){
    qDebug() << "Retriever failed: " << error;
}

