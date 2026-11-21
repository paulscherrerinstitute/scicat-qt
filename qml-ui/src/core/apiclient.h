#pragma once

#include "servers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>

namespace scicat {

// One finished HTTP exchange. Deletes itself after finished() was delivered.
class ApiReply : public QObject
{
    Q_OBJECT
public:
    explicit ApiReply(QNetworkReply *reply, QObject *parent = nullptr);

    int httpStatus() const { return m_httpStatus; }
    bool connectionFailed() const { return m_httpStatus == 0; }
    bool ok() const { return m_networkError == QNetworkReply::NoError && m_httpStatus >= 200 && m_httpStatus < 300; }
    const QByteArray &body() const { return m_body; }
    QJsonDocument json() const { return QJsonDocument::fromJson(m_body); }
    QString errorString() const { return m_errorString; }
    // SciCat answers errors with {"message": ...}; validation errors carry a list of messages
    QString serverMessage() const;
    void abort();

signals:
    void finished(scicat::ApiReply *reply);

private:
    QPointer<QNetworkReply> m_reply;
    int m_httpStatus = 0;
    QNetworkReply::NetworkError m_networkError = QNetworkReply::NoError;
    QByteArray m_body;
    QString m_errorString;
};

class ApiClient;

// All items of a list endpoint, fetched page by page; on Failed, items() holds the pages loaded before the error.
class PagedFetch : public QObject
{
    Q_OBJECT
public:
    enum class Status { Running, Complete, Capped, Failed };

    PagedFetch(ApiClient *client, const QString &endpoint, ApiVersion version, const QJsonObject &filter,
               const QJsonObject &limits);

    Status status() const { return m_status; }
    const QJsonArray &items() const { return m_items; }
    int httpStatus() const { return m_httpStatus; }
    QString errorText() const { return m_errorText; }
    void abort();

signals:
    void progress(int itemsSoFar);
    void finished(scicat::PagedFetch *fetch);

private:
    friend class ApiClient;
    void start();
    void requestPage();
    void finish(Status status);

    ApiClient *m_client;
    QString m_endpoint;
    ApiVersion m_version;
    QJsonObject m_filter;
    QJsonObject m_limits;
    QJsonArray m_items;
    QPointer<ApiReply> m_pending;
    Status m_status = Status::Running;
    int m_page = 0;
    int m_httpStatus = 0;
    QString m_errorText;
};

// Asynchronous access to the SciCat REST API of the selected server.
class ApiClient : public QObject
{
    Q_OBJECT
public:
    // the backend returns at most 100 items per list request unless a limit is given
    static constexpr int defaultPageSize = 500;
    static constexpr int defaultMaxPages = 100;

    explicit ApiClient(QObject *parent = nullptr);

    int serverId() const { return m_serverId; }
    void setServerId(int id);
    // local trials: every server's API base is replaced by this URL, so no real SciCat is contacted
    void setApiOverride(const QString &apiBase);
    bool hasApiOverride() const { return !m_apiOverride.isEmpty(); }
    QString apiBase() const;
    QString token() const { return m_token; }
    void setToken(const QString &token) { m_token = token; }

    QString apiUrl(ApiVersion version, const QString &endpoint) const;
    QNetworkRequest request(const QString &endpoint, ApiVersion version, const QJsonObject &filter = QJsonObject(),
                            const QString &filterKey = QStringLiteral("filter")) const;
    ApiReply *get(const QNetworkRequest &request);
    ApiReply *post(const QNetworkRequest &request, const QByteArray &body);
    PagedFetch *fetchAllPages(const QString &endpoint, ApiVersion version, const QJsonObject &filter,
                              const QJsonObject &limits);

    int pageSize() const { return m_pageSize; }
    int maxPages() const { return m_maxPages; }
    void setPaging(int pageSize, int maxPages);

private:
    QNetworkAccessManager m_network;
    int m_serverId = 0;
    QString m_apiOverride;
    QString m_token;
    int m_pageSize = defaultPageSize;
    int m_maxPages = defaultMaxPages;
};

} // namespace scicat
