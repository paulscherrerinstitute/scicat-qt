#include "networkmanager.h"

NetworkManager::NetworkManager() //: manager(new QNetworkAccessManager(parent))
{
    connect(this, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinishedNM(QNetworkReply *)));
}

NetworkManager::~NetworkManager(){
//    qDebug() << "destroyed";
}


void NetworkManager::replyFinishedNM(QNetworkReply *reply){
    replyString = reply->readAll();
    block = false;
}

QNetworkReply *NetworkManager::getReply() const
{
    return reply;
}

void NetworkManager::post(const QNetworkRequest &request, const QByteArray &postData){
    requestStarted = true;
    reply = this->QNetworkAccessManager::post(request, postData);
}

void NetworkManager::get(const QNetworkRequest &request){
    requestStarted = true;
    reply = this->QNetworkAccessManager::get(request);
}

void NetworkManager::custom(const QNetworkRequest &request, const QByteArray &postData){
    requestStarted = true;
    reply = this->QNetworkAccessManager::sendCustomRequest(request, postData);
}

void NetworkManager::waitForReply() const {
//    qDebug() << replyString;
    if (block){
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
    }

}

QString NetworkManager::getReplyString() const{
    return replyString;
}

QString NetworkManager::startNetworkManagerAndWait(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData){
    NetworkManager* manager = new NetworkManager();
    switch(op){
    case NetworkManager::PostOperation: manager->post(request, postData); break;
    case NetworkManager::GetOperation: manager->get(request); break;
    case NetworkManager::PutOperation: manager->put(request, postData); break;
    case NetworkManager::CustomOperation: manager->custom(request, postData); break;
    default: return "";
    }
    auto doc = manager->pull();
    delete manager;
    return doc;
}

std::shared_ptr<NetworkManager> NetworkManager::startNetworkManager(const NetworkManager::Operation& op, const QNetworkRequest& request, const QByteArray& postData){
    std::shared_ptr<NetworkManager> manager = std::make_shared<NetworkManager>();
    switch(op){
    case NetworkManager::PostOperation: manager->post(request, postData); break;
    case NetworkManager::GetOperation: manager->get(request); break;
    case NetworkManager::PutOperation: manager->put(request, postData); break;
    case NetworkManager::CustomOperation: manager->custom(request, postData); break;
    default: return nullptr;
    }
    return manager;
}

QString NetworkManager::pull(){
    QString doc;

    this->waitForReply();
    doc = this->getReplyString().toUtf8();
    return doc;
}

