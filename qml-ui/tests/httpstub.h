#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <functional>

// A tiny HTTP server for tests: one handler decides the answer of every request. 127.0.0.1 only.
class HttpStub : public QObject
{
    Q_OBJECT
public:
    struct Request
    {
        QByteArray method;
        QString path;
        QJsonObject filter; // the decoded "filter" or "filters" query item
        QByteArray body;
        QByteArray authorization;
    };
    struct Answer
    {
        int status = 200;
        QByteArray body = "[]";
    };
    using Handler = std::function<Answer(const Request &)>;

    explicit HttpStub(Handler handler, QObject *parent = nullptr)
        : QObject(parent)
        , m_handler(std::move(handler))
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] { read(socket); });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QString apiBase() const { return QStringLiteral("http://127.0.0.1:%1/api/").arg(m_server.serverPort()); }
    QList<Request> requests;

private:
    void read(QTcpSocket *socket)
    {
        QByteArray &buffer = m_buffers[socket];
        buffer += socket->readAll();
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;
        const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
        int contentLength = 0;
        Request request;
        for (const QByteArray &rawLine : lines) {
            const QByteArray line = rawLine.trimmed();
            if (line.toLower().startsWith("content-length:"))
                contentLength = line.mid(15).trimmed().toInt();
            if (line.toLower().startsWith("authorization:"))
                request.authorization = line.mid(14).trimmed();
        }
        if (buffer.size() < headerEnd + 4 + contentLength)
            return;
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        request.method = requestLine.value(0);
        const QUrl url = QUrl::fromEncoded(requestLine.value(1));
        request.path = url.path();
        const QUrlQuery query(url);
        QString filter = query.queryItemValue(QStringLiteral("filter"), QUrl::FullyDecoded);
        if (filter.isEmpty())
            filter = query.queryItemValue(QStringLiteral("filters"), QUrl::FullyDecoded);
        request.filter = QJsonDocument::fromJson(filter.toUtf8()).object();
        request.body = buffer.mid(headerEnd + 4, contentLength);
        m_buffers.remove(socket);
        requests.append(request);

        const Answer answer = m_handler(request);
        socket->write("HTTP/1.1 " + QByteArray::number(answer.status) + " X\r\nContent-Type: application/json\r\nContent-Length: "
                      + QByteArray::number(answer.body.size()) + "\r\nConnection: close\r\n\r\n" + answer.body);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    Handler m_handler;
    QHash<QTcpSocket *, QByteArray> m_buffers;
};
