#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "dialog.h"

class NetworkManager:public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkManager();
    ~NetworkManager();
    void post(const QNetworkRequest &request, const QByteArray &postData);
    void get(const QNetworkRequest &request);
    QString pull();
    QString getReply() const;
    static QString startNetworkManagerAndWait(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData="");
    static std::shared_ptr<NetworkManager> startNetworkManager(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData="");

public slots:
    void replyFinishedNM(QNetworkReply *reply);
private:
    bool block = false;
    bool requestStarted = false;
    QString replyString;
    QNetworkReply* reply;
    void waitForReply() const;
};



#endif // NETWORKMANAGER_H
