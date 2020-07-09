#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtNetwork>
#include <QtCore>


class DownloadManager: public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    void append(const QUrl &url);
    void append(const QStringList &urls);
    QString saveFileName(const QUrl &url);
    QString getOutputPath() const;
    void setOutputPath(const QString &value);
    void wait() const;

signals:
    void finished();

private slots:
    void startNextDownload();
    void downloadFinished();
    void downloadReadyRead();

private:
    QNetworkAccessManager manager;
    QQueue<QUrl> downloadQueue;
    QNetworkReply *currentDownload = nullptr;
    QFile output;
    QElapsedTimer downloadTimer;
    QString outputPath;

    int downloadedCount = 0;
    int totalCount = 0;
};

#endif // DOWNLOADMANAGER_H
