#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "dialog.h"

class NetworkManager:public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkManager();
    void post(const QNetworkRequest &request, const QByteArray &postData);
    void get(const QNetworkRequest &request);
    void waitForReply();
    QString getReply();
    static QString startNetworkManager(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData="");

public slots:
    void replyFinishedNM(QNetworkReply *reply);
private:
    bool block = false;
    bool requestStarted = false;
    QString replyString;
    QNetworkReply* reply;
};



#endif // NETWORKMANAGER_H
