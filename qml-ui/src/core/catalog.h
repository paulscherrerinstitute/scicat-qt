#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Request shapes and answer parsing of the SciCat API, without any I/O.
// The filters and bodies are the ones the Qt Widgets GUI sends (PSIArchiver/dialog.cpp, datacatalogue*.cpp).
namespace scicat {

// ---- auth/whoami ----
struct UserInfo
{
    QString id;
    QString username;
    QString email;
    QStringList groups;
};

enum class AuthState { Valid, Invalid, ConnectionError };

AuthState parseWhoami(int httpStatus, const QJsonDocument &answer, UserInfo *user);

// ---- users/<id>/userIdentity ----
// The identity profile holds the institutional email address, the one scicat-cli uses. The address in the answer of
// auth/whoami can be a placeholder of the login provider, so the profile's address is preferred when there is one.
QString userIdentityEndpoint(const QString &userId);
QString parseIdentityEmail(const QJsonDocument &answer);

// something@something.tld without spaces: the check of the Widgets GUI (Experiment::isEmailAddress)
bool isEmailAddress(const QString &value);

// ---- pgroup ----
enum class PgroupProblem { None, Empty, Malformed, NotMember };

PgroupProblem checkPgroup(const QString &pgroup, const QStringList &userGroups);

// ---- proposals ----
struct Proposal
{
    QString piFirstName;
    QString piLastName;
    QString piEmail;
    QString title;
    QString abstract;
    QString instrument;
    QString proposalId;
};

QJsonObject proposalFilter(const QString &pgroup);
bool parseProposals(const QJsonDocument &answer, Proposal *proposal);

// ---- datasets ----
struct DatasetSummary
{
    QString pid;
    QString name;
    QString type;
    QString sourceFolder;
    QDateTime created;
    qint64 sizeBytes = -1;
    int fileCount = -1;
    bool retrievable = false;
    QString archiveStatus;
};

QJsonObject datasetListFilter(const QString &ownerGroup);
QJsonObject datasetPickFilter(const QString &ownerGroup);
QJsonObject datasetListLimits();
DatasetSummary parseDataset(const QJsonObject &dataset);

struct DataFile
{
    QString path;
    qint64 sizeBytes = 0;
    QDateTime time;
};

QJsonObject datasetFilesFilter(const QString &pid);
QJsonObject scientificMetadataFilter();
QList<DataFile> parseDatasetFiles(const QJsonDocument &answer, bool *ok);

// ---- retrieve jobs ----
struct RetrieveJob
{
    QString id;
    QString statusCode;
    QString statusMessage;
    QDateTime created;
    QStringList pids;

    bool succeeded() const { return statusCode == QLatin1String("finishedSuccessful"); }
    bool failed() const { return statusCode.startsWith(QLatin1String("finished")) && !succeeded(); }
    bool pending() const { return !statusCode.startsWith(QLatin1String("finished")); }

    friend bool operator==(const RetrieveJob &a, const RetrieveJob &b)
    {
        return a.id == b.id && a.statusCode == b.statusCode && a.statusMessage == b.statusMessage && a.created == b.created
               && a.pids == b.pids;
    }
};

QJsonObject retrieveJobsFilter(const QString &username);
QJsonObject retrieveJobsLimits();
// the server filters already; this re-check guards against an ignored filter key
bool isRetrieveJobOf(const QJsonObject &job, const QString &username);
RetrieveJob parseRetrieveJob(const QJsonObject &job);
QJsonObject retrieveRequestBody(const QString &username, const QString &email, const QString &ownerGroup,
                                const QStringList &pids);

} // namespace scicat
