#include "networkmanager.h"

NetworkManager::NetworkManager() //: manager(new QNetworkAccessManager(parent))
{
    connect(this, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinishedNM(QNetworkReply *)));

}


void NetworkManager::replyFinishedNM(QNetworkReply *reply){
    replyString = reply->readAll();
    block = false;
}

void NetworkManager::post(const QNetworkRequest &request, const QByteArray &postData){
    requestStarted = true;
    reply = this->QNetworkAccessManager::post(request, postData);
}

void NetworkManager::get(const QNetworkRequest &request){
    requestStarted = true;
    reply = this->QNetworkAccessManager::get(request);
}

void NetworkManager::waitForReply(){
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

QString NetworkManager::getReply(){
    return replyString;
}

QString NetworkManager::startNetworkManager(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData){
    NetworkManager* manager = new NetworkManager();
    switch(op){
    case NetworkManager::PostOperation: manager->post(request, postData); break;
    case NetworkManager::GetOperation: manager->get(request); break;
    default: return "";
    }
    manager->waitForReply();
    QString doc = manager->getReply().toUtf8();
    delete manager;
    return doc;
}

