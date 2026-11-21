#include "catalog.h"

#include <QRegularExpression>
#include <QUrl>

namespace scicat {

namespace {
QDateTime parseTime(const QJsonValue &value)
{
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
}
} // namespace

AuthState parseWhoami(int httpStatus, const QJsonDocument &answer, UserInfo *user)
{
    const bool success = httpStatus >= 200 && httpStatus < 300;
    if (httpStatus == 0 || (success && !answer.isObject()))
        return AuthState::ConnectionError;
    const QJsonObject info = answer.object();
    if (!success || info.value(QLatin1String("username")).toString().isEmpty())
        return AuthState::Invalid;
    if (user) {
        user->id = info.value(QLatin1String("_id")).toString();
        user->username = info.value(QLatin1String("username")).toString();
        user->email = info.value(QLatin1String("email")).toString();
        user->groups.clear();
        const QJsonArray groups = info.value(QLatin1String("currentGroups")).toArray();
        for (const QJsonValue &group : groups)
            user->groups << group.toString();
    }
    return AuthState::Valid;
}

QString userIdentityEndpoint(const QString &userId)
{
    return QStringLiteral("users/") + QString::fromUtf8(QUrl::toPercentEncoding(userId)) + QStringLiteral("/userIdentity");
}

QString parseIdentityEmail(const QJsonDocument &answer)
{
    const QJsonArray emails = answer.object().value(QLatin1String("profile")).toObject().value(QLatin1String("emails")).toArray();
    return emails.isEmpty() ? QString() : emails.first().toObject().value(QLatin1String("value")).toString();
}

bool isEmailAddress(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
    return pattern.match(value).hasMatch();
}

PgroupProblem checkPgroup(const QString &pgroup, const QStringList &userGroups)
{
    if (pgroup.isEmpty())
        return PgroupProblem::Empty;
    const bool lengthFits = pgroup.size() == 6 || pgroup.size() == 7;
    const bool prefixFits = pgroup.startsWith(QLatin1Char('p')) || pgroup.startsWith(QLatin1Char('a'));
    if (!lengthFits || !prefixFits)
        return PgroupProblem::Malformed;
    if (!userGroups.contains(pgroup))
        return PgroupProblem::NotMember;
    return PgroupProblem::None;
}

QJsonObject proposalFilter(const QString &pgroup)
{
    return {{QStringLiteral("where"), QJsonObject{{QStringLiteral("ownerGroup"), pgroup}}}};
}

bool parseProposals(const QJsonDocument &answer, Proposal *proposal)
{
    if (!answer.isArray() || answer.array().isEmpty())
        return false;
    const QJsonObject first = answer.array().first().toObject();
    proposal->piFirstName = first.value(QLatin1String("pi_firstname")).toString();
    proposal->piLastName = first.value(QLatin1String("pi_lastname")).toString();
    proposal->piEmail = first.value(QLatin1String("pi_email")).toString();
    proposal->title = first.value(QLatin1String("title")).toString();
    proposal->abstract = first.value(QLatin1String("abstract")).toString();
    proposal->instrument = first.value(QLatin1String("MeasurementPeriodList")).toArray().first().toObject()
                               .value(QLatin1String("instrument")).toString();
    proposal->proposalId = first.value(QLatin1String("proposalId")).toString();
    return true;
}

QJsonObject datasetListFilter(const QString &ownerGroup)
{
    // "datasetlifecycle", "pid" and "datasetName" are what the Widgets GUI asks for; the rest feeds the extra columns
    return {{QStringLiteral("where"), QJsonObject{{QStringLiteral("ownerGroup"), ownerGroup}}},
            {QStringLiteral("fields"), QJsonArray{QStringLiteral("datasetlifecycle"), QStringLiteral("pid"), QStringLiteral("datasetName"),
                                                  QStringLiteral("size"), QStringLiteral("creationTime"), QStringLiteral("type"),
                                                  QStringLiteral("numberOfFiles"), QStringLiteral("sourceFolder")}}};
}

QJsonObject datasetPickFilter(const QString &ownerGroup)
{
    return {{QStringLiteral("where"), QJsonObject{{QStringLiteral("ownerGroup"), ownerGroup}}},
            {QStringLiteral("fields"), QJsonArray{QStringLiteral("datasetName"), QStringLiteral("pid")}}};
}

QJsonObject datasetListLimits()
{
    // sorted by pid, which is unique and among the requested fields (the v3 endpoint sorts after selecting the fields)
    return {{QStringLiteral("order"), QStringLiteral("pid:asc")}};
}

DatasetSummary parseDataset(const QJsonObject &dataset)
{
    DatasetSummary summary;
    summary.pid = dataset.value(QLatin1String("pid")).toString();
    summary.name = dataset.value(QLatin1String("datasetName")).toString();
    summary.type = dataset.value(QLatin1String("type")).toString();
    summary.sourceFolder = dataset.value(QLatin1String("sourceFolder")).toString();
    summary.created = parseTime(dataset.value(QLatin1String("creationTime")));
    if (dataset.value(QLatin1String("size")).isDouble())
        summary.sizeBytes = qint64(dataset.value(QLatin1String("size")).toDouble());
    if (dataset.value(QLatin1String("numberOfFiles")).isDouble())
        summary.fileCount = dataset.value(QLatin1String("numberOfFiles")).toInt(-1);
    const QJsonObject lifecycle = dataset.value(QLatin1String("datasetlifecycle")).toObject();
    summary.retrievable = lifecycle.value(QLatin1String("retrievable")).toBool();
    summary.archiveStatus = lifecycle.value(QLatin1String("archiveStatusMessage")).toString();
    return summary;
}

QJsonObject datasetFilesFilter(const QString &pid)
{
    return {{QStringLiteral("where"), QJsonObject{{QStringLiteral("pid"), pid}}},
            {QStringLiteral("include"), QJsonArray{QStringLiteral("origdatablocks")}}};
}

QJsonObject scientificMetadataFilter()
{
    return {{QStringLiteral("fields"), QJsonArray{QStringLiteral("scientificMetadata")}}};
}

QList<DataFile> parseDatasetFiles(const QJsonDocument &answer, bool *ok)
{
    QList<DataFile> files;
    const QJsonArray datasets = answer.array();
    const QJsonValue blocks = datasets.isEmpty() ? QJsonValue() : datasets.first().toObject().value(QLatin1String("origdatablocks"));
    if (ok)
        *ok = blocks.isArray();
    const QJsonArray blockList = blocks.toArray();
    for (const QJsonValue &block : blockList) {
        const QJsonArray fileList = block.toObject().value(QLatin1String("dataFileList")).toArray();
        for (const QJsonValue &entry : fileList) {
            const QJsonObject file = entry.toObject();
            // sizes above 2 GiB do not fit an int: read them as double
            files.append({file.value(QLatin1String("path")).toString(), qint64(file.value(QLatin1String("size")).toDouble()),
                          parseTime(file.value(QLatin1String("time")))});
        }
    }
    return files;
}

QJsonObject retrieveJobsFilter(const QString &username)
{
    // The Widgets GUI also filters on statusCode "finishedSuccessful". Leaving that out returns the requested, running
    // and failed jobs as well, which is what lets the list say "requested" instead of nothing.
    return {{QStringLiteral("where"), QJsonObject{{QStringLiteral("type"), QStringLiteral("retrieve")}, {QStringLiteral("ownerUser"), username}}}};
}

QJsonObject retrieveJobsLimits()
{
    return {{QStringLiteral("sort"), QJsonObject{{QStringLiteral("createdAt"), QStringLiteral("desc")}}}};
}

bool isRetrieveJobOf(const QJsonObject &job, const QString &username)
{
    return job.value(QLatin1String("type")).toString() == QLatin1String("retrieve")
           && job.value(QLatin1String("ownerUser")).toString() == username;
}

RetrieveJob parseRetrieveJob(const QJsonObject &job)
{
    RetrieveJob result;
    result.id = job.value(QLatin1String("id")).toString();
    if (result.id.isEmpty())
        result.id = job.value(QLatin1String("_id")).toString();
    result.statusCode = job.value(QLatin1String("statusCode")).toString();
    result.statusMessage = job.value(QLatin1String("statusMessage")).toString();
    result.created = parseTime(job.value(QLatin1String("createdAt")));
    const QJsonArray datasets = job.value(QLatin1String("jobParams")).toObject().value(QLatin1String("datasetList")).toArray();
    for (const QJsonValue &dataset : datasets)
        result.pids << dataset.toObject().value(QLatin1String("pid")).toString();
    return result;
}

QJsonObject retrieveRequestBody(const QString &username, const QString &email, const QString &ownerGroup,
                                const QStringList &pids)
{
    QJsonArray datasetList;
    for (const QString &pid : pids)
        datasetList.append(QJsonObject{{QStringLiteral("pid"), pid}, {QStringLiteral("files"), QJsonArray()}});

    QJsonObject jobParams;
    jobParams[QLatin1String("username")] = username;
    jobParams[QLatin1String("destinationPath")] = QStringLiteral("/archive/retrieve");
    jobParams[QLatin1String("option")] = QStringLiteral("PSI");
    jobParams[QLatin1String("datasetList")] = datasetList;

    QJsonObject request;
    request[QLatin1String("type")] = QStringLiteral("retrieve");
    request[QLatin1String("jobParams")] = jobParams;
    request[QLatin1String("ownerUser")] = username;
    request[QLatin1String("ownerGroup")] = ownerGroup;
    if (!email.isEmpty())
        request[QLatin1String("contactEmail")] = email;
    return request;
}

} // namespace scicat
