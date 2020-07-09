#include "downloadmanager.h"
#include <QTextStream>


DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
{
}

void DownloadManager::append(const QStringList &urls)
{
    for (auto url : urls){
        append(QUrl::fromEncoded(url.toLocal8Bit()));
    }

    if (downloadQueue.isEmpty()){
        QTimer::singleShot(0, this, &DownloadManager::finished);
    }
}

void DownloadManager::append(const QUrl &url)
{
    if (downloadQueue.isEmpty())
            QTimer::singleShot(0, this, &DownloadManager::startNextDownload);
    downloadQueue.enqueue(url);
    ++totalCount;
}

QString DownloadManager::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QDir(outputPath).filePath(QFileInfo(path).fileName());

    if (basename.isEmpty())
        basename = "download";

    return basename;
}

void DownloadManager::startNextDownload()
{
    if (downloadQueue.isEmpty()) {
        emit finished();
        return;
    }

    QUrl url = downloadQueue.dequeue();

    QString filename = saveFileName(url);
    output.setFileName(filename);
    if (!output.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file for writing";
        startNextDownload();
        return;                 // skip this download
    }

    QNetworkRequest request(url);
    currentDownload = manager.get(request);
    connect(currentDownload, &QNetworkReply::finished,
            this, &DownloadManager::downloadFinished);
    connect(currentDownload, &QNetworkReply::readyRead,
            this, &DownloadManager::downloadReadyRead);

    // prepare the output
    downloadTimer.start();
}


void DownloadManager::downloadFinished()
{
    output.close();

    if (currentDownload->error()) {
        // download failed
        qDebug() << "Download failed";
        output.remove();
    }
    output.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser);
    currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    output.write(currentDownload->readAll());
}

void DownloadManager::setOutputPath(const QString &value)
{
    outputPath = value;
}

QString DownloadManager::getOutputPath() const
{
    return outputPath;
}

void DownloadManager::wait() const{

    QEventLoop loop;
    connect(this, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

}
