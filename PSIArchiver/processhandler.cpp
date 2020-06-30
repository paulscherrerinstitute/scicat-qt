#include "processhandler.h"

#include "QDebug"

ProcessHandler::ProcessHandler(QWidget *parent)
{
    job = new QProcess(parent);

    job->setProcessChannelMode(QProcess::MergedChannels);
    connect(job, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(processFailed(QProcess::ProcessError)));
    connect(job, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int)));
    connect(job, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));


}

ProcessHandler::~ProcessHandler(){
    qDebug()<< "deleted ProcessHandler";
}

void ProcessHandler::setOutputStream(QTextEdit* out){
    m_outStream = out;
}

void ProcessHandler::readOutput(){
    qDebug() << "reading output";
    while(job->canReadLine()){
        auto tmp = job->readLine();

        // redirect output stream
        if (m_outStream!=nullptr){
            m_outStream->append(tmp);
        }
        qDebug() << tmp;
    }
}

void ProcessHandler::processFinished(int val){

    qDebug() << "Process finished with exit code " << val;
    if (val==0){
        emit finished(val);
    }

}

void ProcessHandler::processFailed(QProcess::ProcessError error){
    qDebug() << "Job failed: " << error;
}

QProcess *ProcessHandler::getJob() const
{
    return job;
}

void ProcessHandler::start(QString cmd, QStringList arguments)
{
    job->start(cmd, arguments);
}





