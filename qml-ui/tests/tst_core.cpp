#include "apiclient.h"
#include "appsettings.h"
#include "catalog.h"
#include "clitool.h"
#include "datasetmetadata.h"
#include "filescan.h"
#include "formatting.h"
#include "httpstub.h"
#include "proposalstore.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace scicat;

namespace {
QByteArray compact(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

// extractSourceFolder() of PSIArchiver/dialog.cpp (non-Windows branch), copied to pin where the two agree
QString legacySourceFolder(const QStringList &container)
{
    QString lPath = QStringLiteral("/");
    int ii = 1;
    bool entryIsValid = true;
    QString subdir;
    while (ii < container[0].length()) {
        for (const QString &v : container) {
            if (v.length() == ii)
                entryIsValid = false;
        }
        if (!entryIsValid)
            break;
        if (container[0][ii] == QLatin1Char('/')) {
            lPath.push_back(subdir);
            subdir.clear();
        }
        subdir.push_back(container[0][ii]);
        ii++;
    }
    return lPath;
}

// the file listing loop of DatasetIngestor::prepareIngestion()
QStringList legacyFileListing(QStringList paths, const QString &sourceFolder)
{
    QStringList lines;
    for (QString v : paths) {
        QString name = v.remove(sourceFolder);
        if (name.startsWith(QLatin1Char('/')))
            name.remove(0, 1);
        lines << name;
    }
    return lines;
}
} // namespace

class CoreTest : public QObject
{
    Q_OBJECT

private slots:
    void pgroup_data();
    void pgroup();
    void whoami();
    void emailAddresses_data();
    void emailAddresses();
    void proposals();
    void requestShapes();
    void requestUrl();
    void sourceFolderMatchesWidgetsGui_data();
    void sourceFolderMatchesWidgetsGui();
    void sourceFolderOfSeparateFolders();
    void metadataRaw();
    void metadataDerived();
    void datasetNames();
    void creationLocation();
    void cliArguments();
    void settings();
    void proposalFile();
    void ingestFiles();
    void fileScan();
    void datasetFilesAboveTwoGiB();
    void retrieveJobs();
    void pagedFetchComplete();
    void pagedFetchCapped();
    void pagedFetchFailsMidway();
    void serverMessages();
    void formatting();
};

void CoreTest::pgroup_data()
{
    QTest::addColumn<QString>("group");
    QTest::addColumn<int>("problem");
    QTest::newRow("empty") << QString() << int(PgroupProblem::Empty);
    QTest::newRow("too short") << QStringLiteral("p123") << int(PgroupProblem::Malformed);
    QTest::newRow("wrong prefix") << QStringLiteral("x12345") << int(PgroupProblem::Malformed);
    QTest::newRow("too long") << QStringLiteral("p1234567") << int(PgroupProblem::Malformed);
    QTest::newRow("member") << QStringLiteral("p12345") << int(PgroupProblem::None);
    QTest::newRow("agroup, seven characters") << QStringLiteral("a-12345") << int(PgroupProblem::None);
    QTest::newRow("not a member") << QStringLiteral("p99999") << int(PgroupProblem::NotMember);
}

void CoreTest::pgroup()
{
    QFETCH(QString, group);
    QFETCH(int, problem);
    QCOMPARE(int(checkPgroup(group, {QStringLiteral("p12345"), QStringLiteral("a-12345")})), problem);
}

void CoreTest::whoami()
{
    UserInfo user;
    QCOMPARE(parseWhoami(0, QJsonDocument(), &user), AuthState::ConnectionError);
    // a proxy answering 200 with something that is not a user record
    QCOMPARE(parseWhoami(200, QJsonDocument(QJsonArray()), &user), AuthState::ConnectionError);
    QCOMPARE(parseWhoami(401, QJsonDocument::fromJson(R"({"message":"Unauthorized"})"), &user), AuthState::Invalid);
    QCOMPARE(parseWhoami(200, QJsonDocument::fromJson(R"({"username":""})"), &user), AuthState::Invalid);
    QCOMPARE(parseWhoami(200, QJsonDocument::fromJson(R"({"_id":"65f0 a/b","username":"jdoe","email":"j@x.ch","currentGroups":["p12345","p22222"]})"), &user),
             AuthState::Valid);
    QCOMPARE(user.id, QStringLiteral("65f0 a/b"));
    QCOMPARE(user.username, QStringLiteral("jdoe"));
    QCOMPARE(user.email, QStringLiteral("j@x.ch"));
    QCOMPARE(user.groups, QStringList({QStringLiteral("p12345"), QStringLiteral("p22222")}));

    // User::requestIdentityMail() of the Widgets GUI: the first address of the identity profile, asked for by user ID
    QCOMPARE(userIdentityEndpoint(user.id), QStringLiteral("users/65f0%20a%2Fb/userIdentity"));
    QCOMPARE(parseIdentityEmail(QJsonDocument::fromJson(R"({"profile":{"emails":[{"value":"john.doe@psi.ch"},{"value":"other@x.ch"}]}})")),
             QStringLiteral("john.doe@psi.ch"));
    QCOMPARE(parseIdentityEmail(QJsonDocument::fromJson(R"({"profile":{"emails":[]}})")), QString());
    QCOMPARE(parseIdentityEmail(QJsonDocument::fromJson(R"({"message":"not found"})")), QString());
    QCOMPARE(parseIdentityEmail(QJsonDocument()), QString());
}

void CoreTest::emailAddresses_data()
{
    // Experiment::isEmailAddress() of the Widgets GUI
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("address");
    QTest::newRow("plain") << "jane.doe@psi.ch" << true;
    QTest::newRow("subdomain") << "j@mail.example.org" << true;
    QTest::newRow("empty") << "" << false;
    QTest::newRow("a name") << "Jane Doe" << false;
    QTest::newRow("masked") << "*****" << false;
    QTest::newRow("no dot") << "jane@localhost" << false;
    QTest::newRow("space inside") << "jane doe@psi.ch" << false;
    QTest::newRow("two at signs") << "jane@doe@psi.ch" << false;
    QTest::newRow("space around") << " jane@psi.ch" << false;
}

void CoreTest::emailAddresses()
{
    QFETCH(QString, text);
    QFETCH(bool, address);
    QCOMPARE(isEmailAddress(text), address);
}

void CoreTest::proposals()
{
    Proposal proposal;
    QVERIFY(!parseProposals(QJsonDocument::fromJson("[]"), &proposal));
    QVERIFY(!parseProposals(QJsonDocument::fromJson(R"({"message":"x"})"), &proposal));
    QVERIFY(parseProposals(QJsonDocument::fromJson(R"([{"proposalId":"20.500/abc","pi_firstname":"Jane","pi_lastname":"Doe","pi_email":"jane@x.ch",
        "title":"T","abstract":"A","MeasurementPeriodList":[{"instrument":"/PSI/SLS/TOMCAT"}]}])"), &proposal));
    QCOMPARE(proposal.piFirstName, QStringLiteral("Jane"));
    QCOMPARE(proposal.piLastName, QStringLiteral("Doe"));
    QCOMPARE(proposal.piEmail, QStringLiteral("jane@x.ch"));
    QCOMPARE(proposal.instrument, QStringLiteral("/PSI/SLS/TOMCAT"));
    QCOMPARE(proposal.proposalId, QStringLiteral("20.500/abc"));
    // a proposal without measurement periods must not crash and leaves the instrument empty
    QVERIFY(parseProposals(QJsonDocument::fromJson(R"([{"proposalId":"p","pi_firstname":"J"}])"), &proposal));
    QCOMPARE(proposal.instrument, QString());
}

void CoreTest::requestShapes()
{
    // the filters and the job body the Widgets GUI sends (dialog.cpp, datacatalogue*.cpp)
    QCOMPARE(compact(proposalFilter(QStringLiteral("p12345"))), QByteArray(R"({"where":{"ownerGroup":"p12345"}})"));
    QCOMPARE(compact(datasetPickFilter(QStringLiteral("p12345"))),
             QByteArray(R"({"fields":["datasetName","pid"],"where":{"ownerGroup":"p12345"}})"));
    QCOMPARE(compact(datasetListLimits()), QByteArray(R"({"order":"pid:asc"})"));
    QCOMPARE(compact(datasetFilesFilter(QStringLiteral("20.500/x"))),
             QByteArray(R"({"include":["origdatablocks"],"where":{"pid":"20.500/x"}})"));
    QCOMPARE(compact(scientificMetadataFilter()), QByteArray(R"({"fields":["scientificMetadata"]})"));
    QCOMPARE(compact(retrieveJobsLimits()), QByteArray(R"({"sort":{"createdAt":"desc"}})"));
    QCOMPARE(compact(retrieveRequestBody(QStringLiteral("jdoe"), QStringLiteral("j@x.ch"), QStringLiteral("p12345"),
                                         {QStringLiteral("20.500/a"), QStringLiteral("20.500/b")})),
             QByteArray(R"({"contactEmail":"j@x.ch","jobParams":{"datasetList":[{"files":[],"pid":"20.500/a"},{"files":[],"pid":"20.500/b"}],)"
                        R"("destinationPath":"/archive/retrieve","option":"PSI","username":"jdoe"},"ownerGroup":"p12345","ownerUser":"jdoe","type":"retrieve"})"));
    QVERIFY(!retrieveRequestBody(QStringLiteral("jdoe"), QString(), QStringLiteral("p12345"), {}).contains(QLatin1String("contactEmail")));

    // deliberate differences: more dataset fields for the extra columns, and all retrieve jobs instead of the finished ones
    const QJsonObject list = datasetListFilter(QStringLiteral("p12345"));
    for (const char *field : {"datasetlifecycle", "pid", "datasetName"})
        QVERIFY(list.value(QLatin1String("fields")).toArray().contains(QLatin1String(field)));
    QCOMPARE(compact(retrieveJobsFilter(QStringLiteral("jdoe"))), QByteArray(R"({"where":{"ownerUser":"jdoe","type":"retrieve"}})"));
}

void CoreTest::requestUrl()
{
    ApiClient client;
    client.setServerId(1);
    QCOMPARE(client.apiUrl(ApiVersion::V3, QStringLiteral("auth/whoami")), QStringLiteral("https://dacat-qa.psi.ch/api/v3/auth/whoami"));
    QCOMPARE(client.apiUrl(ApiVersion::V4, QStringLiteral("jobs")), QStringLiteral("https://dacat-qa.psi.ch/api/v4/jobs"));
    client.setServerId(7); // stale ID
    QCOMPARE(client.serverId(), 0);
    client.setApiOverride(QStringLiteral("http://127.0.0.1:1/api"));
    client.setToken(QStringLiteral("tok"));
    const QNetworkRequest request = client.request(QStringLiteral("proposals"), ApiVersion::V3, proposalFilter(QStringLiteral("p1")), QStringLiteral("filters"));
    QCOMPARE(request.url().toEncoded(), QByteArray("http://127.0.0.1:1/api/v3/proposals?filters=%7B%22where%22%3A%7B%22ownerGroup%22%3A%22p1%22%7D%7D"));
    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer tok"));
    QCOMPARE(request.rawHeader("Accept"), QByteArray("application/json"));
}

void CoreTest::sourceFolderMatchesWidgetsGui_data()
{
    QTest::addColumn<QStringList>("paths");
    QTest::addColumn<QString>("folder");
    QTest::newRow("one folder") << QStringList{"/das/work/p12/p12345/run1"} << "/das/work/p12/p12345";
    QTest::newRow("one file") << QStringList{"/das/work/p12/p12345/run1/a.h5"} << "/das/work/p12/p12345/run1";
    QTest::newRow("siblings") << QStringList{"/data/run1", "/data/run2", "/data/notes.txt"} << "/data";
    QTest::newRow("siblings, first one longest") << QStringList{"/data/a_long_name.h5", "/data/b.h5"} << "/data";
    QTest::newRow("file at the root") << QStringList{"/file.h5"} << "/";
}

void CoreTest::sourceFolderMatchesWidgetsGui()
{
    QFETCH(QStringList, paths);
    QFETCH(QString, folder);
    QCOMPARE(commonSourceFolder(paths), folder);
    QCOMPARE(commonSourceFolder(paths), legacySourceFolder(paths));
    QCOMPARE(fileListing(paths, folder), legacyFileListing(paths, folder));
}

void CoreTest::sourceFolderOfSeparateFolders()
{
    const QStringList paths{"/data/run1/a.h5", "/data/run2/sub/b.h5"};
    // the Widgets GUI takes the first entry's parent and then lists the second file with a path that does not exist below it
    QCOMPARE(legacySourceFolder(paths), QStringLiteral("/data/run1"));
    QCOMPARE(legacyFileListing(paths, legacySourceFolder(paths)), QStringList({"a.h5", "data/run2/sub/b.h5"}));
    // here both files are listed below their real common folder
    QCOMPARE(commonSourceFolder(paths), QStringLiteral("/data"));
    QCOMPARE(fileListing(paths, QStringLiteral("/data")), QStringList({"run1/a.h5", "run2/sub/b.h5"}));
    // only the leading folder is cut off, a repeated folder name deeper down stays
    QCOMPARE(fileListing({"/a/x/a/y.h5"}, QStringLiteral("/a")), QStringList({"x/a/y.h5"}));
    QCOMPARE(legacyFileListing({"/a/x/a/y.h5"}, QStringLiteral("/a")), QStringList({"x/y.h5"}));
    QCOMPARE(commonSourceFolder({"/a/b/c.h5", "/x/y.h5"}), QStringLiteral("/"));
}

void CoreTest::metadataRaw()
{
    DatasetDraft draft;
    draft.raw = true;
    draft.datasetName = QStringLiteral("run_1");
    draft.description = QStringLiteral("d");
    draft.owner = QStringLiteral("Jane Doe");
    draft.ownerGroup = QStringLiteral("p12345");
    draft.ownerEmail = QStringLiteral("jane@x.ch");
    draft.proposalId = QStringLiteral("20.500.11935/abc");
    draft.principalInvestigator = QStringLiteral("jane@x.ch");
    draft.creationLocation = QStringLiteral("/PSI/SLS/TOMCAT");
    draft.scientificMetadata = QJsonObject{{"sample", QJsonObject{{"name", "S1"}, {"description", ""}, {"principalInvestigator", ""}}}};
    draft.investigator = QStringLiteral("ignored for raw data");
    QJsonObject expected = QJsonDocument::fromJson(R"({
        "sourceFolder": "/data", "owner": "Jane Doe", "ownerGroup": "p12345", "ownerEmail": "jane@x.ch", "description": "d",
        "datasetName": "run_1", "proposalId": "20.500.11935/abc", "principalInvestigator": "jane@x.ch",
        "creationLocation": "/PSI/SLS/TOMCAT", "type": "raw",
        "scientificMetadata": {"sample": {"name": "S1", "description": "", "principalInvestigator": ""}}})").object();
    QCOMPARE(metadataJson(draft, QStringLiteral("/data")), expected);

    // DatasetIngestor::prepareIngestion() of the Widgets GUI leaves out an empty owner email (scicat-cli then takes the
    // address of the token's owner) and an empty proposal ID; the principal investigator may be a name
    draft.ownerEmail.clear();
    draft.proposalId.clear();
    draft.principalInvestigator = QStringLiteral("Jane Doe");
    expected.remove("ownerEmail");
    expected.remove("proposalId");
    expected["principalInvestigator"] = "Jane Doe";
    QCOMPARE(metadataJson(draft, QStringLiteral("/data")), expected);
}

void CoreTest::metadataDerived()
{
    DatasetDraft draft;
    draft.raw = false;
    draft.datasetName = QStringLiteral("recon");
    draft.owner = QStringLiteral("Jane Doe");
    draft.ownerGroup = QStringLiteral("p12345");
    draft.ownerEmail = QStringLiteral("jane@x.ch");
    draft.investigator = QStringLiteral("jdoe@x.ch");
    draft.scientificMetadata = QJsonObject{{"dropped", "for derived data, as in the Widgets GUI"}};
    QJsonObject expected = QJsonDocument::fromJson(R"({
        "sourceFolder": "/data", "owner": "Jane Doe", "ownerGroup": "p12345", "ownerEmail": "jane@x.ch", "description": "",
        "datasetName": "recon", "type": "derived", "investigator": "jdoe@x.ch", "inputDatasets": ["N/A"], "usedSoftware": ["N/A"]})").object();
    QCOMPARE(metadataJson(draft, QStringLiteral("/data")), expected);

    draft.inputDatasets = {QStringLiteral("20.500/a"), QStringLiteral("20.500/b")};
    draft.usedSoftware = QStringLiteral("https://gitlab.psi.ch/x@1\nsecond line");
    expected["inputDatasets"] = QJsonArray{"20.500/a", "20.500/b"};
    expected["usedSoftware"] = QJsonArray{"https://gitlab.psi.ch/x@1\nsecond line"};
    QCOMPARE(metadataJson(draft, QStringLiteral("/data")), expected);
}

void CoreTest::datasetNames()
{
    QCOMPARE(normalizedDatasetName(QStringLiteral("my demo run")), QStringLiteral("my_demo_run"));
    QCOMPARE(guessDatasetName({}), QString());
    QCOMPARE(guessDatasetName({QStringLiteral("/data/scan_0001.tar.h5")}), QStringLiteral("scan_0001"));
    QCOMPARE(guessDatasetName({QStringLiteral("/data/sample-data/scan_0001"), QStringLiteral("/data/sample-data/notes.txt")}),
             QStringLiteral("sample-data"));
}

void CoreTest::creationLocation()
{
    QString error;
    QCOMPARE(customCreationLocation(QStringLiteral("psi"), QStringLiteral("sls"), QStringLiteral("Tomcat"), &error), QStringLiteral("/PSI/SLS/TOMCAT"));
    QCOMPARE(customCreationLocation(QStringLiteral("eth"), QString(), QStringLiteral("lab1"), &error), QStringLiteral("/ETH/LAB1"));
    QCOMPARE(customCreationLocation(QString(), QStringLiteral("sls"), QStringLiteral("x"), &error), QString());
    QCOMPARE(error, QStringLiteral("institute"));
    QCOMPARE(customCreationLocation(QStringLiteral("psi"), QString(), QStringLiteral(" "), &error), QString());
    QCOMPARE(error, QStringLiteral("instrument"));
}

void CoreTest::cliArguments()
{
    // DatasetIngestor::start() of the Widgets GUI; the token is passed on every platform
    QCOMPARE(ingestArguments(true, true, 1, QStringLiteral("tok"), QStringLiteral("/l/metadata.json"), QStringLiteral("/l/filelisting.txt")),
             QStringList({"datasetIngestor", "--ingest", "--autoarchive", "--testenv", "--noninteractive", "--allowexistingsource",
                          "--token", "tok", "/l/metadata.json", "/l/filelisting.txt"}));
    QCOMPARE(ingestArguments(false, false, 0, QStringLiteral("tok"), QStringLiteral("m"), QStringLiteral("f")),
             QStringList({"datasetIngestor", "--noninteractive", "--allowexistingsource", "--token", "tok", "m", "f"}));
    QCOMPARE(ingestArguments(true, false, 2, QStringLiteral("tok"), QStringLiteral("m"), QStringLiteral("f")).mid(0, 3),
             QStringList({"datasetIngestor", "--ingest", "--devenv"}));
    // DatasetRetriever::start() of the Widgets GUI
    QCOMPARE(retrieveArguments(1, QStringLiteral("20.500/a"), QStringLiteral("tok"), QStringLiteral("/out dir")),
             QStringList({"datasetRetriever", "--testenv", "--dataset", "20.500/a", "--token", "tok", "--retrieve", "/out dir"}));
    QCOMPARE(displayCommand(QStringLiteral("scicat-cli"), retrieveArguments(0, QStringLiteral("p"), QStringLiteral("secret"), QStringLiteral("/out dir"))),
             QStringLiteral("scicat-cli datasetRetriever --dataset p --token <hidden> --retrieve \"/out dir\""));
}

void CoreTest::settings()
{
    QTemporaryDir dir;
    const QString ini = dir.filePath(QStringLiteral("settings.ini"));
    {
        AppSettings settings(ini);
        // defaults of defaultSettings_t in the Widgets GUI
        QCOMPARE(settings.value(QStringLiteral("serverID")).toInt(), 0);
        QCOMPARE(settings.string(QStringLiteral("filenameMetadata")), QStringLiteral("metadata.json"));
        QCOMPARE(settings.string(QStringLiteral("filenameFilelisting")), QStringLiteral("filelisting.txt"));
        QCOMPARE(settings.string(QStringLiteral("logPath")), QDir::homePath() + QStringLiteral("/.scicat/log"));
        for (const char *on : {"printIngestion", "usePgroupAsDefaultPath", "saveProposalInfo", "loadProposalInfo", "saveLog", "enableArchive",
                               "enableIngest", "requestAlreadyRetrievedData", "guessDatasetname"})
            QVERIFY2(settings.boolean(QLatin1String(on)), on);
        for (const char *off : {"deleteLog", "disableSystemCalls", "investigatorIsPI"})
            QVERIFY2(!settings.boolean(QLatin1String(off)), off);
        settings.setValue(QStringLiteral("deleteLog"), true);
        settings.setValue(QStringLiteral("serverID"), 1);
        settings.setUiValue(QStringLiteral("lastPgroup"), QStringLiteral("p12345"));
    }
    {
        AppSettings settings(ini);
        // an ini file stores text: the values still come back as bool and int
        QCOMPARE(settings.value(QStringLiteral("deleteLog")), QVariant(true));
        QCOMPARE(settings.value(QStringLiteral("serverID")), QVariant(1));
        const QVariantMap before = settings.snapshot();
        settings.setValue(QStringLiteral("scicatCliBinary"), QStringLiteral("/opt/scicat/scicat-cli"));
        settings.resetToDefaults();
        QVERIFY(!settings.boolean(QStringLiteral("deleteLog")));
        // the server and the scicat-cli that was found are not preferences: a reset leaves them
        QCOMPARE(settings.value(QStringLiteral("serverID")), QVariant(1));
        QCOMPARE(settings.value(QStringLiteral("scicatCliBinary")).toString(), QStringLiteral("/opt/scicat/scicat-cli"));
        QCOMPARE(settings.uiValue(QStringLiteral("lastPgroup")).toString(), QStringLiteral("p12345"));
        settings.restore(before);
        QVERIFY(settings.boolean(QStringLiteral("deleteLog")));
    }
    {
        // a store written by another major version is emptied
        QSettings raw(ini, QSettings::IniFormat);
        raw.setValue(QStringLiteral("version"), QStringLiteral("9.0"));
        raw.sync();
        AppSettings settings(ini);
        QVERIFY(!settings.boolean(QStringLiteral("deleteLog")));
    }
    QCOMPARE(AppSettings::fixedFileName(QStringLiteral(" meta "), QStringLiteral(".json"), QStringLiteral("metadata.json")), QStringLiteral("meta.json"));
    QCOMPARE(AppSettings::fixedFileName(QStringLiteral("META.JSON"), QStringLiteral(".json"), QStringLiteral("metadata.json")), QStringLiteral("META.JSON"));
    QCOMPARE(AppSettings::fixedFileName(QString(), QStringLiteral(".txt"), QStringLiteral("filelisting.txt")), QStringLiteral("filelisting.txt"));
}

void CoreTest::proposalFile()
{
    QTemporaryDir dir;
    Proposal proposal;
    QVERIFY(!loadProposalFile(dir.path(), QStringLiteral("p22222"), &proposal));
    proposal.piFirstName = QStringLiteral("Jane");
    proposal.piLastName = QStringLiteral("Doe");
    proposal.piEmail = QStringLiteral("jane@x.ch");
    QVERIFY(saveProposalFile(dir.path(), QStringLiteral("p22222"), proposal));
    QCOMPARE(proposalFilePath(dir.path(), QStringLiteral("p22222")), dir.path() + QStringLiteral("/proposals/proposal_p22222.dat"));
    QFile file(proposalFilePath(dir.path(), QStringLiteral("p22222")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("Jane\nDoe\njane@x.ch\n"));
    Proposal loaded;
    QVERIFY(loadProposalFile(dir.path(), QStringLiteral("p22222"), &loaded));
    QCOMPARE(loaded.piEmail, QStringLiteral("jane@x.ch"));
    QCOMPARE(loaded.proposalId, QString());
}

void CoreTest::ingestFiles()
{
    QTemporaryDir dir;
    DatasetDraft draft;
    draft.raw = true;
    draft.datasetName = QStringLiteral("run_1");
    draft.ownerGroup = QStringLiteral("p12345");
    draft.paths = {QStringLiteral("/data/run1"), QStringLiteral("/data/notes.txt")};
    const IngestFiles files = writeIngestFiles(draft, dir.path(), QStringLiteral("metadata.json"), QStringLiteral("filelisting.txt"), 1700000000);
    QVERIFY2(files.ok(), qPrintable(files.error));
    QCOMPARE(files.directory, dir.path() + QStringLiteral("/ingestion/run_11700000000"));
    QFile metadata(files.metadataFile);
    QVERIFY(metadata.open(QIODevice::ReadOnly));
    const QJsonObject written = QJsonDocument::fromJson(metadata.readAll()).object();
    QCOMPARE(written.value("sourceFolder").toString(), QStringLiteral("/data"));
    QCOMPARE(written.value("type").toString(), QStringLiteral("raw"));
    QFile listing(files.fileListingFile);
    QVERIFY(listing.open(QIODevice::ReadOnly));
    QCOMPARE(listing.readAll(), QByteArray("run1\nnotes.txt\n"));

    QFile blocker(dir.filePath(QStringLiteral("blocked")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    QVERIFY(!writeIngestFiles(draft, blocker.fileName(), QStringLiteral("m.json"), QStringLiteral("f.txt"), 1).ok());
}

void CoreTest::fileScan()
{
    QTemporaryDir dir;
    QDir(dir.path()).mkpath(QStringLiteral("a/b"));
    for (const char *name : {"a/1.h5", "a/b/2.h5", "a/b/3.h5", "a/.hidden"}) {
        QFile file(dir.filePath(QLatin1String(name)));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("12345");
    }
    const ScanResult folder = scanPath(dir.filePath(QStringLiteral("a")));
    QVERIFY(folder.exists && folder.isFolder);
    QCOMPARE(folder.files, 4);
    QCOMPARE(folder.bytes, 20);
    const ScanResult file = scanPath(dir.filePath(QStringLiteral("a/1.h5")));
    QVERIFY(file.exists && !file.isFolder);
    QCOMPARE(file.files, 1);
    QVERIFY(!scanPath(dir.filePath(QStringLiteral("missing"))).exists);
    std::atomic_bool cancel{true};
    QVERIFY(scanPath(dir.filePath(QStringLiteral("a")), &cancel).cancelled);
}

void CoreTest::datasetFilesAboveTwoGiB()
{
    bool ok = false;
    const QList<DataFile> files = parseDatasetFiles(QJsonDocument::fromJson(R"([{"pid":"p","origdatablocks":[
        {"dataFileList":[{"path":"raw/a.h5","size":4294967296,"time":"2026-01-05T15:00:00.000Z"}]},
        {"dataFileList":[{"path":"raw/b.h5","size":12}]}]}])"), &ok);
    QVERIFY(ok);
    QCOMPARE(files.size(), 2);
    QCOMPARE(files.at(0).sizeBytes, Q_INT64_C(4294967296));
    QCOMPARE(files.at(0).time.toUTC().toString(Qt::ISODate), QStringLiteral("2026-01-05T15:00:00Z"));
    parseDatasetFiles(QJsonDocument::fromJson(R"({"message":"Forbidden"})"), &ok);
    QVERIFY(!ok);
    parseDatasetFiles(QJsonDocument::fromJson("[]"), &ok);
    QVERIFY(!ok);

    const DatasetSummary summary = parseDataset(QJsonDocument::fromJson(R"({"pid":"p","datasetName":"n","type":"raw","size":18329108480,
        "numberOfFiles":85,"creationTime":"2026-02-27T10:00:00.000Z","datasetlifecycle":{"retrievable":true,"archiveStatusMessage":"datasetOnArchiveDisk"}})").object());
    QCOMPARE(summary.sizeBytes, Q_INT64_C(18329108480));
    QCOMPARE(summary.fileCount, 85);
    QVERIFY(summary.retrievable);
    QVERIFY(summary.created.isValid());
    QCOMPARE(parseDataset(QJsonObject{{"pid", "p"}}).sizeBytes, -1);
}

void CoreTest::retrieveJobs()
{
    const QJsonObject job = QJsonDocument::fromJson(R"({"id":"j1","type":"retrieve","ownerUser":"jdoe","statusCode":"inProgress",
        "createdAt":"2026-10-01T00:00:01.000Z","jobParams":{"datasetList":[{"pid":"a","files":[]},{"pid":"b","files":[]}]}})").object();
    QVERIFY(isRetrieveJobOf(job, QStringLiteral("jdoe")));
    QVERIFY(!isRetrieveJobOf(job, QStringLiteral("someone")));
    RetrieveJob parsed = parseRetrieveJob(job);
    QCOMPARE(parsed.pids, QStringList({"a", "b"}));
    QVERIFY(parsed.pending() && !parsed.succeeded() && !parsed.failed());
    parsed.statusCode = QStringLiteral("finishedSuccessful");
    QVERIFY(parsed.succeeded() && !parsed.pending() && !parsed.failed());
    parsed.statusCode = QStringLiteral("finishedUnsuccessful");
    QVERIFY(parsed.failed() && !parsed.pending());
    parsed.statusCode = QStringLiteral("jobSubmitted");
    QVERIFY(parsed.pending());
}

void CoreTest::pagedFetchComplete()
{
    // 5 items in pages of 2: three requests, the last one short
    HttpStub stub([](const HttpStub::Request &request) {
        const QJsonObject limits = request.filter.value("limits").toObject();
        QJsonArray page;
        for (int i = limits.value("skip").toInt(); i < qMin(5, limits.value("skip").toInt() + limits.value("limit").toInt()); ++i)
            page.append(QJsonObject{{"pid", QString::number(i)}});
        return HttpStub::Answer{200, QJsonDocument(page).toJson()};
    });
    ApiClient client;
    client.setApiOverride(stub.apiBase());
    client.setToken(QStringLiteral("good"));
    client.setPaging(2, 100);
    PagedFetch *fetch = client.fetchAllPages(QStringLiteral("datasets"), ApiVersion::V3, datasetPickFilter(QStringLiteral("p1")), datasetListLimits());
    QSignalSpy finished(fetch, &PagedFetch::finished);
    QSignalSpy progress(fetch, &PagedFetch::progress);
    int items = -1;
    PagedFetch::Status status = PagedFetch::Status::Running;
    connect(fetch, &PagedFetch::finished, this, [&](PagedFetch *f) { items = int(f->items().size()); status = f->status(); });
    QVERIFY(finished.wait(5000));
    QCOMPARE(status, PagedFetch::Status::Complete);
    QCOMPARE(items, 5);
    QCOMPARE(stub.requests.size(), 3);
    QCOMPARE(progress.size(), 3);
    QCOMPARE(stub.requests.at(0).path, QStringLiteral("/api/v3/datasets"));
    QCOMPARE(stub.requests.at(0).authorization, QByteArray("Bearer good"));
    QCOMPARE(compact(stub.requests.at(1).filter.value("limits").toObject()), QByteArray(R"({"limit":2,"order":"pid:asc","skip":2})"));
    QCOMPARE(stub.requests.at(1).filter.value("where").toObject().value("ownerGroup").toString(), QStringLiteral("p1"));
}

void CoreTest::pagedFetchCapped()
{
    HttpStub stub([](const HttpStub::Request &) { return HttpStub::Answer{200, R"([{"pid":"a"},{"pid":"b"}])"}; });
    ApiClient client;
    client.setApiOverride(stub.apiBase());
    client.setPaging(2, 3);
    PagedFetch *fetch = client.fetchAllPages(QStringLiteral("jobs"), ApiVersion::V4, {}, {});
    PagedFetch::Status status = PagedFetch::Status::Running;
    int items = -1;
    connect(fetch, &PagedFetch::finished, this, [&](PagedFetch *f) { items = int(f->items().size()); status = f->status(); });
    QSignalSpy finished(fetch, &PagedFetch::finished);
    QVERIFY(finished.wait(5000));
    QCOMPARE(status, PagedFetch::Status::Capped);
    QCOMPARE(items, 6);
    QCOMPARE(stub.requests.size(), 3);
    QCOMPARE(stub.requests.at(0).path, QStringLiteral("/api/v4/jobs"));
}

void CoreTest::pagedFetchFailsMidway()
{
    int calls = 0;
    HttpStub stub([&calls](const HttpStub::Request &) {
        return ++calls == 1 ? HttpStub::Answer{200, R"([{"pid":"a"},{"pid":"b"}])"} : HttpStub::Answer{403, R"({"message":"Forbidden resource"})"};
    });
    ApiClient client;
    client.setApiOverride(stub.apiBase());
    client.setPaging(2, 100);
    PagedFetch *fetch = client.fetchAllPages(QStringLiteral("datasets"), ApiVersion::V3, {}, {});
    PagedFetch::Status status = PagedFetch::Status::Running;
    int items = -1;
    QString error;
    connect(fetch, &PagedFetch::finished, this, [&](PagedFetch *f) { items = int(f->items().size()); status = f->status(); error = f->errorText(); });
    QSignalSpy finished(fetch, &PagedFetch::finished);
    QVERIFY(finished.wait(5000));
    QCOMPARE(status, PagedFetch::Status::Failed);
    QCOMPARE(items, 2); // the pages loaded before the error are kept
    QCOMPARE(error, QStringLiteral("Forbidden resource"));
}

void CoreTest::serverMessages()
{
    int mode = 0;
    HttpStub stub([&mode](const HttpStub::Request &) {
        switch (mode) {
        case 0: return HttpStub::Answer{403, R"({"message":"Invalid new job. User needs to belong to job owner group.","error":"Forbidden","statusCode":403})"};
        case 1: return HttpStub::Answer{400, R"({"message":["ownerGroup must be a string","type should not be empty"]})"};
        case 2: return HttpStub::Answer{500, R"({"error":{"message":"old style message"}})"};
        default: return HttpStub::Answer{201, R"({"id":"new"})"};
        }
    });
    ApiClient client;
    client.setApiOverride(stub.apiBase());
    const QStringList expected{"Invalid new job. User needs to belong to job owner group.", "ownerGroup must be a string; type should not be empty", "old style message"};
    for (mode = 0; mode < 4; ++mode) {
        QNetworkRequest request = client.request(QStringLiteral("jobs"), ApiVersion::V4);
        request.setRawHeader("Content-Type", "application/json");
        ApiReply *reply = client.post(request, "{}");
        QString message;
        bool ok = false;
        int status = 0;
        connect(reply, &ApiReply::finished, this, [&](ApiReply *r) { message = r->serverMessage(); ok = r->ok(); status = r->httpStatus(); });
        QSignalSpy finished(reply, &ApiReply::finished);
        QVERIFY(finished.wait(5000));
        if (mode < 3) {
            QVERIFY(!ok);
            QCOMPARE(message, expected.at(mode));
        } else {
            QVERIFY(ok);
            QCOMPARE(status, 201);
        }
    }
    // nothing listens on this port: a connection error, not an HTTP error
    client.setApiOverride(QStringLiteral("http://127.0.0.1:9/api/"));
    ApiReply *reply = client.get(client.request(QStringLiteral("auth/whoami"), ApiVersion::V3));
    bool connectionFailed = false;
    connect(reply, &ApiReply::finished, this, [&](ApiReply *r) { connectionFailed = r->connectionFailed(); });
    QSignalSpy finished(reply, &ApiReply::finished);
    QVERIFY(finished.wait(10000));
    QVERIFY(connectionFailed);
}

void CoreTest::formatting()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    QCOMPARE(formatBytes(-1), QString());
    QCOMPARE(formatBytes(512), QStringLiteral("512 bytes"));
    QCOMPARE(formatBytes(Q_INT64_C(4294967296)), QStringLiteral("4.0 GiB"));
    QCOMPARE(formatCount(150000), QStringLiteral("150,000"));
    QCOMPARE(formatDuration(42), QStringLiteral("42 s"));
    QCOMPARE(formatDuration(185), QStringLiteral("3 min 5 s"));
    QCOMPARE(formatDuration(7800), QStringLiteral("2 h 10 min"));
    QCOMPARE(formatDateTime(QDateTime()), QString());
}

QTEST_GUILESS_MAIN(CoreTest)
#include "tst_core.moc"
