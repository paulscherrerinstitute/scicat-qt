#include "apiclient.h"

#include <QUrl>

namespace scicat {

ApiReply::ApiReply(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , m_reply(reply)
{
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        m_httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_networkError = reply->error();
        m_errorString = reply->errorString();
        if (reply->isOpen())
            m_body = reply->readAll();
        emit finished(this);
        deleteLater();
    });
}

QString ApiReply::serverMessage() const
{
    const QJsonObject body = json().object();
    QString message = body.value(QLatin1String("error")).toObject().value(QLatin1String("message")).toString();
    if (message.isEmpty()) {
        const QJsonValue value = body.value(QLatin1String("message"));
        QStringList messages;
        const QJsonArray list = value.isArray() ? value.toArray() : QJsonArray{value};
        for (const QJsonValue &entry : list)
            messages << entry.toString();
        message = messages.join(QStringLiteral("; ")).trimmed();
    }
    return message.isEmpty() ? m_errorString : message;
}

void ApiReply::abort()
{
    if (m_reply)
        m_reply->abort();
}

PagedFetch::PagedFetch(ApiClient *client, const QString &endpoint, ApiVersion version, const QJsonObject &filter,
                       const QJsonObject &limits)
    : QObject(client)
    , m_client(client)
    , m_endpoint(endpoint)
    , m_version(version)
    , m_filter(filter)
    , m_limits(limits)
{
}

void PagedFetch::start()
{
    requestPage();
}

void PagedFetch::abort()
{
    if (m_status != Status::Running)
        return;
    disconnect(this, &PagedFetch::finished, nullptr, nullptr);
    if (m_pending) {
        disconnect(m_pending, nullptr, this, nullptr);
        m_pending->abort();
    }
    m_status = Status::Failed;
    deleteLater();
}

void PagedFetch::requestPage()
{
    const int pageSize = m_client->pageSize();
    QJsonObject limits = m_limits;
    limits[QLatin1String("limit")] = pageSize;
    limits[QLatin1String("skip")] = m_page * pageSize;
    QJsonObject filter = m_filter;
    filter[QLatin1String("limits")] = limits;

    m_pending = m_client->get(m_client->request(m_endpoint, m_version, filter));
    connect(m_pending, &ApiReply::finished, this, [this, pageSize](ApiReply *reply) {
        const QJsonDocument document = reply->json();
        m_httpStatus = reply->httpStatus();
        if (!reply->ok() || !document.isArray()) {
            m_errorText = reply->connectionFailed() ? reply->errorString() : reply->serverMessage();
            qWarning().noquote() << "[ApiClient]" << m_endpoint << "request failed (HTTP" << m_httpStatus << "):" << m_errorText;
            finish(Status::Failed);
            return;
        }
        const QJsonArray pageItems = document.array();
        for (const QJsonValue &item : pageItems)
            m_items.append(item);
        emit progress(int(m_items.size()));
        if (pageItems.size() < pageSize) {
            finish(Status::Complete);
            return;
        }
        if (++m_page >= m_client->maxPages()) {
            qWarning() << "[ApiClient]" << m_endpoint << "stopped after" << m_client->maxPages() << "pages";
            finish(Status::Capped);
            return;
        }
        requestPage();
    });
}

void PagedFetch::finish(Status status)
{
    m_status = status;
    emit finished(this);
    deleteLater();
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
}

void ApiClient::setServerId(int id)
{
    m_serverId = Servers::validId(id);
}

void ApiClient::setApiOverride(const QString &apiBase)
{
    m_apiOverride = apiBase;
    if (!m_apiOverride.isEmpty() && !m_apiOverride.endsWith(QLatin1Char('/')))
        m_apiOverride += QLatin1Char('/');
}

// A test backend can be told which server was meant: "{server}" in the override becomes production, qa or development.
QString ApiClient::apiBase() const
{
    if (m_apiOverride.isEmpty())
        return Servers::at(m_serverId).apiBase;
    QString base = m_apiOverride;
    return base.replace(QLatin1String("{server}"), Servers::at(m_serverId).name.toLower());
}

QString ApiClient::apiUrl(ApiVersion version, const QString &endpoint) const
{
    return apiBase() + (version == ApiVersion::V4 ? QLatin1String("v4/") : QLatin1String("v3/")) + endpoint;
}

QNetworkRequest ApiClient::request(const QString &endpoint, ApiVersion version, const QJsonObject &filter,
                                   const QString &filterKey) const
{
    QByteArray url = apiUrl(version, endpoint).toUtf8();
    if (!filter.isEmpty())
        url += "?" + filterKey.toUtf8() + "=" + QUrl::toPercentEncoding(QString::fromUtf8(QJsonDocument(filter).toJson(QJsonDocument::Compact)));
    QNetworkRequest request(QUrl::fromEncoded(url));
    request.setRawHeader("Accept", "application/json");
    if (!m_token.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setTransferTimeout(QNetworkRequest::DefaultTransferTimeout);
    return request;
}

// SCICAT_QML_TRACE=1 prints every request the application sends: the verb and the address without its query,
// so never the token or a filter. Every request goes through get() or post().
static void traceRequest(const char *verb, const QNetworkRequest &request)
{
    static const bool enabled = qEnvironmentVariableIsSet("SCICAT_QML_TRACE");
    if (enabled)
        qInfo().noquote() << "[request]" << verb << request.url().toString(QUrl::RemoveQuery | QUrl::RemoveUserInfo);
}

ApiReply *ApiClient::get(const QNetworkRequest &request)
{
    traceRequest("GET", request);
    return new ApiReply(m_network.get(request), this);
}

ApiReply *ApiClient::post(const QNetworkRequest &request, const QByteArray &body)
{
    traceRequest("POST", request);
    return new ApiReply(m_network.post(request, body), this);
}

PagedFetch *ApiClient::fetchAllPages(const QString &endpoint, ApiVersion version, const QJsonObject &filter,
                                     const QJsonObject &limits)
{
    auto *fetch = new PagedFetch(this, endpoint, version, filter, limits);
    // start after the caller had the chance to connect
    QMetaObject::invokeMethod(fetch, [fetch] { fetch->start(); }, Qt::QueuedConnection);
    return fetch;
}

void ApiClient::setPaging(int pageSize, int maxPages)
{
    m_pageSize = pageSize;
    m_maxPages = maxPages;
}

} // namespace scicat
