#include "backend.h"
#include "httpstub.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

// A SciCat small enough to read at a glance: user jdoe, groups p12345 (proposal, four datasets) and p22222 (nothing).
class FakeCatalogue
{
public:
    QList<QJsonObject> postedJobs;
    bool failDatasets = false;
    bool failJobs = false;
    bool rejectJobs = false;
    bool extraDataset = false;
    int datasetListRequests = 0;
    QString identityEmail;                              // empty: the user has no identity profile (404)
    int identityRequests = 0;
    QString piEmail = QStringLiteral("jane@example.org"); // of the proposal of p12345

    HttpStub::Answer answer(const HttpStub::Request &request)
    {
        if (request.authorization != "Bearer good")
            return {401, R"({"message":"Unauthorized","statusCode":401})"};
        if (request.path.endsWith("/auth/whoami"))
            return {200, R"({"_id":"user-1","username":"jdoe","email":"jdoe@example.org","currentGroups":["p12345","p22222"]})"};
        if (request.path == "/api/v3/users/user-1/userIdentity") {
            ++identityRequests;
            if (identityEmail.isEmpty())
                return {404, R"({"message":"not found","statusCode":404})"};
            return {200, QJsonDocument(QJsonObject{{"profile", QJsonObject{{"emails", QJsonArray{QJsonObject{{"value", identityEmail}}}}}}}).toJson()};
        }
        const QString group = request.filter.value("where").toObject().value("ownerGroup").toString();
        if (request.path == "/api/v3/proposals") {
            if (group != "p12345")
                return {200, "[]"};
            return {200, QStringLiteral(R"([{"proposalId":"20.500/prop","pi_firstname":"Jane","pi_lastname":"Doe","pi_email":"%1",
                "title":"Demo","abstract":"A","MeasurementPeriodList":[{"instrument":"/PSI/SLS/DEMO"}]}])").arg(piEmail).toUtf8()};
        }
        if (request.path == "/api/v4/jobs" && request.method == "GET" && failJobs)
            return {500, R"({"message":"database down"})"};
        if (request.path == "/api/v4/jobs" && request.method == "GET") {
            QJsonArray jobs{QJsonDocument::fromJson(R"({"id":"j2","type":"retrieve","ownerUser":"jdoe","statusCode":"finishedUnsuccessful",
                    "statusMessage":"tape error","createdAt":"2026-10-02T00:00:00.000Z","jobParams":{"datasetList":[{"pid":"pid/c"}]}})").object(),
                             QJsonDocument::fromJson(R"({"id":"j1","type":"retrieve","ownerUser":"jdoe","statusCode":"finishedSuccessful",
                    "createdAt":"2026-10-01T00:00:00.000Z","jobParams":{"datasetList":[{"pid":"pid/b"}]}})").object()};
            for (const QJsonObject &job : std::as_const(postedJobs))
                jobs.prepend(job);
            return {200, QJsonDocument(jobs).toJson()};
        }
        if (request.path == "/api/v4/jobs" && request.method == "POST") {
            if (rejectJobs)
                return {403, R"({"message":"Invalid new job. User needs to belong to job owner group.","statusCode":403})"};
            QJsonObject job = QJsonDocument::fromJson(request.body).object();
            job["id"] = "posted";
            job["statusCode"] = "jobSubmitted";
            job["createdAt"] = "2026-11-16T10:00:00.000Z";
            postedJobs.append(job);
            return {201, QJsonDocument(job).toJson()};
        }
        if (request.path == "/api/v3/datasets") {
            if (failDatasets)
                return {500, R"({"message":"database down"})"};
            if (request.filter.value("where").toObject().contains("pid"))
                return {200, R"([{"pid":"pid/a","origdatablocks":[{"dataFileList":[{"path":"b.h5","size":4294967296,"time":"2026-01-05T15:00:00.000Z"},
                    {"path":"a.h5","size":10,"time":"2026-01-05T15:00:00.000Z"}]}]}])"};
            if (group != "p12345")
                return {200, "[]"};
            ++datasetListRequests;
            if (extraDataset)
                return {200, R"([
                {"pid":"pid/a","datasetName":"alpha","type":"raw","size":4294967306,"numberOfFiles":2,"creationTime":"2026-01-01T10:00:00.000Z","datasetlifecycle":{"retrievable":true}},
                {"pid":"pid/e","datasetName":"epsilon","type":"raw","size":1,"creationTime":"2026-01-05T10:00:00.000Z","datasetlifecycle":{"retrievable":false}}])"};
            return {200, R"([
                {"pid":"pid/a","datasetName":"alpha","type":"raw","size":4294967306,"numberOfFiles":2,"creationTime":"2026-01-01T10:00:00.000Z","datasetlifecycle":{"retrievable":true}},
                {"pid":"pid/b","datasetName":"beta","type":"raw","size":100,"creationTime":"2026-01-02T10:00:00.000Z","datasetlifecycle":{"retrievable":true}},
                {"pid":"pid/c","datasetName":"gamma","type":"derived","size":50,"creationTime":"2026-01-03T10:00:00.000Z","datasetlifecycle":{"retrievable":true}},
                {"pid":"pid/d","datasetName":"delta","type":"raw","size":7,"creationTime":"2026-01-04T10:00:00.000Z","datasetlifecycle":{"retrievable":false}}])"};
        }
        if (request.path == "/api/v3/datasets/pid/a")
            return {200, R"({"scientificMetadata":{"beam":{"energy":{"value":12.41,"unit":"keV"}},"detector":"eiger","cooled":true,"runs":[1,2]}})"};
        return {404, R"({"message":"not found"})"};
    }
};

class BackendTest : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<FakeCatalogue> m_catalogue;
    std::unique_ptr<HttpStub> m_stub;
    std::unique_ptr<Backend> m_backend;

    void signIn()
    {
        m_backend->session()->signIn(QStringLiteral(" good "));
        QTRY_VERIFY(m_backend->session()->signedIn());
    }
    void openGroup(const QString &group)
    {
        QVERIFY(m_backend->workspace()->select(group));
        QTRY_COMPARE(m_backend->workspace()->state(), Workspace::Ready);
    }

private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        m_catalogue = std::make_unique<FakeCatalogue>();
        m_stub = std::make_unique<HttpStub>([this](const HttpStub::Request &request) { return m_catalogue->answer(request); });
        BackendConfig config;
        config.settingsFile = m_dir->filePath(QStringLiteral("settings.ini"));
        config.apiOverride = m_stub->apiBase();
        config.applicationDir = m_dir->path();
        m_backend = std::make_unique<Backend>(config);
        m_backend->settings()->setLogPath(m_dir->filePath(QStringLiteral("log")));
        m_backend->settings()->setCliPath(QStringLiteral("/nonexistent/scicat-cli"));
    }
    void cleanup()
    {
        m_backend.reset();
        m_stub.reset();
    }

    void signInOutcomes();
    void groupWithProposal();
    void groupWithoutProposalRemembersPi();
    void datasetStatesAndFilters();
    void requestSendsTheWidgetsBody();
    void requestRejected();
    void listFailureIsAnErrorState();
    void detailsKeepNumbersAndLargeSizes();
    void draftValidationAndSnapshot();
    void failedRunKeepsTheDraft();
    void metadataModelEditing();
    void metadataImportExport();
    void signOutForgetsEverything();
    void reloadTwiceStillLoads();
    void ticksAndCurrentDatasetFollowTheData();
    void requestsAreKnownWithoutTheRetrievePage();
    void signOutWhileARunIsActive();
    void pickerStartsCleanAndCurrent();
    void undoOfAResetEndsWithTheNextChange();
    void sendOptionsFollowTheSettingsUntilTouched();
    void activityAsksForTheRequestsOnce();
    void emailComesFromTheIdentityProfile();
    void ownerFieldsFollowTheProposalUntilTyped();
    void maskedPiEmailFallsBackToTheUser();
    void typedPiEmailHasToBeAnAddress();
};

void BackendTest::signInOutcomes()
{
    Session *session = m_backend->session();
    QCOMPARE(session->environment(), Session::LocalTest);
    session->signIn(QStringLiteral("   "));
    QCOMPARE(session->problem(), Session::TokenMissing);
    QCOMPARE(session->state(), Session::SignedOut);

    session->signIn(QStringLiteral("wrong"));
    QCOMPARE(session->state(), Session::SigningIn);
    QTRY_COMPARE(session->problem(), Session::TokenRejected);
    QCOMPARE(session->state(), Session::SignedOut);
    QVERIFY(session->token().isEmpty());

    signIn();
    QCOMPARE(session->problem(), Session::NoProblem);
    QCOMPARE(session->username(), QStringLiteral("jdoe"));
    QCOMPARE(session->groups(), QStringList({"p12345", "p22222"}));
    // signing in asks who the token belongs to and nothing else: what a page needs is asked for when it opens
    QTest::qWait(50);
    QStringList paths;
    for (const HttpStub::Request &request : std::as_const(m_stub->requests))
        paths << request.path;
    QVERIFY(paths.contains(QStringLiteral("/api/v3/auth/whoami")));
    QVERIFY(!paths.contains(QStringLiteral("/api/v4/jobs")));

    // nothing answers on the other server: a connection problem, not a rejected token
    session->signOut();
    m_backend->api()->setApiOverride(QStringLiteral("http://127.0.0.1:9/api/"));
    session->signIn(QStringLiteral("good"));
    QTRY_COMPARE_WITH_TIMEOUT(session->problem(), Session::NoConnection, 10000);
}

void BackendTest::groupWithProposal()
{
    signIn();
    Workspace *workspace = m_backend->workspace();
    QVERIFY(!workspace->problemWith(QStringLiteral("p123")).isEmpty());
    QVERIFY(!workspace->problemWith(QStringLiteral("p99999")).isEmpty());
    QVERIFY(!workspace->select(QStringLiteral("p99999")));
    QCOMPARE(workspace->state(), Workspace::NoGroup);

    openGroup(QStringLiteral("p12345"));
    QCOMPARE(workspace->source(), Workspace::FromCatalog);
    QCOMPARE(workspace->owner(), QStringLiteral("Jane Doe"));
    QCOMPARE(workspace->instrument(), QStringLiteral("/PSI/SLS/DEMO"));
    QVERIFY(workspace->archiveReady());
    QVERIFY(!workspace->piEditable());
    workspace->setPiEmail(QStringLiteral("other@x.ch")); // catalogue data cannot be overwritten
    QCOMPARE(workspace->piEmail(), QStringLiteral("jane@example.org"));
    // the proposal is asked for with the "filters" key, as the Widgets GUI does
    const auto proposals = std::find_if(m_stub->requests.crbegin(), m_stub->requests.crend(),
                                        [](const HttpStub::Request &request) { return request.path == QLatin1String("/api/v3/proposals"); });
    QVERIFY(proposals != m_stub->requests.crend());
    QCOMPARE(proposals->filter.value("where").toObject().value("ownerGroup").toString(), QStringLiteral("p12345"));
    QCOMPARE(workspace->recentGroups(), QStringList{"p12345"});

    // the proposal's instrument is offered and chosen although it is not in the built-in list
    QCOMPARE(m_backend->archive()->creationLocation(), QStringLiteral("/PSI/SLS/DEMO"));
    QVERIFY(m_backend->archive()->instruments().contains(QStringLiteral("/PSI/SLS/DEMO")));
    QCOMPARE(m_backend->archive()->investigator(), QStringLiteral("jdoe@example.org"));
}

void BackendTest::groupWithoutProposalRemembersPi()
{
    signIn();
    Workspace *workspace = m_backend->workspace();
    openGroup(QStringLiteral("p22222"));
    QCOMPARE(workspace->source(), Workspace::Manual);
    QVERIFY(workspace->piEditable());
    QVERIFY(workspace->groupReady());
    QVERIFY(!workspace->archiveReady());
    workspace->setPiFirstName(QStringLiteral("Max"));
    workspace->setPiLastName(QStringLiteral("Muster"));
    workspace->setPiEmail(QStringLiteral("max@x.ch"));
    QVERIFY(workspace->archiveReady());
    QCOMPARE(workspace->rememberPi(), QString());
    QVERIFY(QFile::exists(m_dir->filePath(QStringLiteral("log/proposals/proposal_p22222.dat"))));

    // next time the saved details come back
    openGroup(QStringLiteral("p12345"));
    openGroup(QStringLiteral("p22222"));
    QCOMPARE(workspace->source(), Workspace::FromSavedFile);
    QCOMPARE(workspace->owner(), QStringLiteral("Max Muster"));

    // with "use PI as investigator" the default investigator follows the PI
    m_backend->settings()->setInvestigatorIsPI(true);
    openGroup(QStringLiteral("p12345"));
    QCOMPARE(m_backend->archive()->investigator(), QStringLiteral("jane@example.org"));
}

void BackendTest::datasetStatesAndFilters()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    DatasetFilterModel *list = retrieve->datasets();
    retrieve->ensureLoaded();
    QCOMPARE(retrieve->loadState(), RetrieveController::Loading);
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    QCOMPARE(retrieve->totalCount(), 4);
    // NotArchived, InArchive, Requested, Ready, RequestFailed
    QCOMPARE(retrieve->stateCounts(), QList<int>({1, 1, 0, 1, 1}));

    const auto stateOf = [&](const QString &pid) { return list->data(list->index(list->rowOfPid(pid), 0), DatasetListModel::StateRole).toInt(); };
    QCOMPARE(stateOf("pid/a"), int(DatasetListModel::InArchive));
    QCOMPARE(stateOf("pid/b"), int(DatasetListModel::Ready));
    QCOMPARE(stateOf("pid/c"), int(DatasetListModel::RequestFailed));
    QCOMPARE(stateOf("pid/d"), int(DatasetListModel::NotArchived));

    // newest first by default
    QCOMPARE(list->pidAt(0), QStringLiteral("pid/d"));
    list->sortBy(DatasetFilterModel::ByName);
    QCOMPARE(list->pidAt(0), QStringLiteral("pid/a"));
    list->sortBy(DatasetFilterModel::ByName);
    QCOMPARE(list->pidAt(0), QStringLiteral("pid/c")); // gamma
    list->sortBy(DatasetFilterModel::BySize);
    QCOMPARE(list->pidAt(0), QStringLiteral("pid/a"));

    list->setSearch(QStringLiteral("ALP"));
    QCOMPARE(list->count(), 1);
    list->setSearch(QStringLiteral("pid/"));
    QCOMPARE(list->count(), 4);
    list->setStateFilter(DatasetListModel::Ready);
    QCOMPARE(list->count(), 1);
    QVERIFY(list->filtered());
    list->clearFilters();
    QCOMPARE(list->count(), 4);

    // selection: only what is listed is selected by "select all"
    list->setSearch(QStringLiteral("a")); // alpha, beta, gamma, delta all contain an a
    list->setSearch(QStringLiteral("et")); // beta
    retrieve->setListedSelected(true);
    QCOMPARE(retrieve->selectedCount(), 1);
    QCOMPARE(retrieve->filteredSelection(), 2);
    list->clearFilters();
    QCOMPARE(retrieve->filteredSelection(), 1);
    QCOMPARE(retrieve->copyableCount(), 1);
    QCOMPARE(retrieve->requestableCount(), 1); // already retrieved, but "request again" is on by default
    m_backend->settings()->setRequestAlreadyRetrieved(false);
    QCOMPARE(retrieve->requestableCount(), 0);
}

void BackendTest::requestSendsTheWidgetsBody()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    retrieve->setListedSelected(true);
    m_backend->settings()->setRequestAlreadyRetrieved(false);
    QCOMPARE(retrieve->selectedCount(), 4);
    QCOMPARE(retrieve->requestableCount(), 2); // alpha (in archive) and gamma (failed before)

    retrieve->requestSelected();
    QVERIFY(retrieve->requesting());
    QTRY_VERIFY(!retrieve->requesting());
    QCOMPARE(retrieve->noticeTone(), QStringLiteral("success"));
    QVERIFY2(retrieve->noticeText().contains(QStringLiteral("Requested 2 datasets")), qPrintable(retrieve->noticeText()));
    QVERIFY(retrieve->noticeText().contains(QStringLiteral("1 not in the archive yet")));
    QVERIFY(retrieve->noticeText().contains(QStringLiteral("1 already retrieved")));
    QCOMPARE(retrieve->selectedCount(), 0);

    QCOMPARE(m_catalogue->postedJobs.size(), 1);
    QJsonObject expected = QJsonDocument::fromJson(R"({"type":"retrieve","ownerUser":"jdoe","ownerGroup":"p12345","contactEmail":"jdoe@example.org",
        "jobParams":{"username":"jdoe","destinationPath":"/archive/retrieve","option":"PSI",
        "datasetList":[{"pid":"pid/a","files":[]},{"pid":"pid/c","files":[]}]}})").object();
    QJsonObject posted = m_catalogue->postedJobs.first();
    for (const char *added : {"id", "statusCode", "createdAt"})
        posted.remove(QLatin1String(added));
    QCOMPARE(posted, expected);

    // the list shows the request without a reload, and the job list follows
    QTRY_COMPARE(retrieve->stateCounts(), QList<int>({1, 0, 2, 1, 0}));
    QTRY_COMPARE(m_backend->activity()->count(), 3);
    const QModelIndex newest = m_backend->activity()->index(0);
    QCOMPARE(newest.data(ActivityModel::StateRole).toInt(), int(ActivityModel::Waiting));
    QCOMPARE(newest.data(ActivityModel::TitleRole).toString(), QStringLiteral("Request alpha and 1 more"));
}

void BackendTest::requestRejected()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    m_catalogue->rejectJobs = true;
    retrieve->toggle(retrieve->datasets()->rowOfPid(QStringLiteral("pid/a")));
    retrieve->requestSelected();
    QTRY_COMPARE(retrieve->noticeTone(), QStringLiteral("error"));
    QCOMPARE(retrieve->noticeText(), QStringLiteral("SciCat rejected the retrieve request (HTTP 403): Invalid new job. User needs to belong to job owner group."));
    QCOMPARE(retrieve->selectedCount(), 1); // the selection is kept for another try
    QCOMPARE(retrieve->stateCounts().at(DatasetListModel::Requested), 0);

    // nothing eligible: said without asking the server
    retrieve->clearSelection();
    retrieve->toggle(retrieve->datasets()->rowOfPid(QStringLiteral("pid/d")));
    const int requests = int(m_stub->requests.size());
    retrieve->requestSelected();
    QCOMPARE(retrieve->noticeTone(), QStringLiteral("warning"));
    QCOMPARE(int(m_stub->requests.size()), requests);
}

void BackendTest::listFailureIsAnErrorState()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    m_catalogue->failDatasets = true;
    RetrieveController *retrieve = m_backend->retrieve();
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Failed);
    QVERIFY2(retrieve->errorText().contains(QStringLiteral("database down")), qPrintable(retrieve->errorText()));
    m_catalogue->failDatasets = false;
    retrieve->reload();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    QCOMPARE(retrieve->totalCount(), 4);

    // an empty pgroup is not an error
    openGroup(QStringLiteral("p22222"));
    QCOMPARE(retrieve->loadState(), RetrieveController::Idle);
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    QCOMPARE(retrieve->totalCount(), 0);
}

void BackendTest::detailsKeepNumbersAndLargeSizes()
{
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    DatasetDetails *details = retrieve->details();
    QVERIFY(!details->available());
    retrieve->setCurrentPid(QStringLiteral("pid/a"));
    QVERIFY(details->available());
    QCOMPARE(details->name(), QStringLiteral("alpha"));
    QCOMPARE(details->sizeText(), QStringLiteral("4.0 GiB"));
    QTRY_COMPARE(details->filesState(), DatasetDetails::Loaded);
    QTRY_COMPARE(details->metadataState(), DatasetDetails::Loaded);

    // files sorted by path; the 4 GiB file keeps its size (the Widgets GUI shows 0)
    QCOMPARE(details->files()->rowCount(), 2);
    QCOMPARE(details->files()->index(0).data(FileListModel::PathRole).toString(), QStringLiteral("a.h5"));
    QCOMPARE(details->files()->index(1).data(FileListModel::SizeTextRole).toString(), QStringLiteral("4.0 GiB"));
    QCOMPARE(details->filesNote(), QStringLiteral("2 files · 4.0 GiB"));

    // numbers and booleans are shown (the Widgets GUI leaves them blank)
    MetadataModel *metadata = details->metadata();
    QStringList lines;
    for (int row = 0; row < metadata->rowCount(); ++row)
        lines << QStringLiteral("%1%2=%3").arg(QString(metadata->index(row).data(MetadataModel::DepthRole).toInt(), QLatin1Char('.')),
                                              metadata->index(row).data(MetadataModel::KeyRole).toString(),
                                              metadata->index(row).data(MetadataModel::ValueRole).toString());
    QCOMPARE(lines, QStringList({"beam=", ".energy=", "..unit=keV", "..value=12.41", "cooled=true", "detector=eiger", "runs=", ".[0]=1", ".[1]=2"}));

    // a second look is answered from memory
    const int requests = int(m_stub->requests.size());
    retrieve->setCurrentPid(QString());
    retrieve->setCurrentPid(QStringLiteral("pid/a"));
    // changes in quick succession are shown once the user stops on a dataset
    QTRY_COMPARE(details->filesState(), DatasetDetails::Loaded);
    QCOMPARE(int(m_stub->requests.size()), requests);
}

void BackendTest::draftValidationAndSnapshot()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    ArchiveDraft *draft = m_backend->archive();
    QDir(m_dir->path()).mkpath(QStringLiteral("data/scan 1"));
    for (const char *name : {"data/scan 1/a.h5", "data/scan 1/b.h5", "data/notes.txt"}) {
        QFile file(m_dir->filePath(QLatin1String(name)));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
    }

    // nothing is reported before the user tried to go on
    QCOMPARE(draft->filesProblem(), QString());
    QVERIFY(!draft->checkStep(ArchiveDraft::FilesStep));
    QVERIFY(!draft->filesProblem().isEmpty());
    QCOMPARE(draft->files()->addPaths({m_dir->filePath("data/scan 1"), m_dir->filePath("data/notes.txt"), m_dir->filePath("data/scan 1/")}), 2);
    QTRY_VERIFY(!draft->files()->scanning());
    QCOMPARE(draft->files()->totalFiles(), 3);
    QCOMPARE(draft->filesProblem(), QString());
    QVERIFY(draft->checkStep(ArchiveDraft::FilesStep));
    QCOMPARE(draft->files()->sourceFolder(), QDir::toNativeSeparators(m_dir->filePath("data")));

    // entering the details suggests a name from the common folder
    draft->setStep(ArchiveDraft::DetailsStep);
    QCOMPARE(draft->datasetName(), QStringLiteral("data"));
    draft->setDatasetName(QStringLiteral(" my demo run "));
    QCOMPARE(draft->datasetName(), QStringLiteral("my_demo_run"));

    // raw data needs an instrument
    draft->setCreationLocation(QString());
    QCOMPARE(draft->instrumentProblem(), QString());
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    QVERIFY(!draft->instrumentProblem().isEmpty());
    QCOMPARE(draft->addInstrument(QString(), QString(), QStringLiteral("x")), QStringLiteral("institute"));
    QCOMPARE(draft->addInstrument(QStringLiteral("psi"), QStringLiteral("sls"), QStringLiteral("tomcat2")), QString());
    QCOMPARE(draft->creationLocation(), QStringLiteral("/PSI/SLS/TOMCAT2"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));

    // a metadata entry without a name blocks, and says so
    const int row = draft->metadata()->addField(-1);
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    QVERIFY(!draft->metadataProblem().isEmpty());
    draft->metadata()->setKey(row, QStringLiteral("energy"));
    draft->metadata()->setKind(row, MetadataModel::Number);
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    draft->metadata()->setValue(row, QStringLiteral("12.4"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));
    QVERIFY(draft->problems().isEmpty());

    const scicat::DatasetDraft snapshot = draft->snapshot();
    const QJsonObject metadata = scicat::metadataJson(snapshot, scicat::commonSourceFolder(snapshot.paths));
    const QJsonObject expected = QJsonDocument::fromJson(QStringLiteral(R"({
        "sourceFolder": "%1", "owner": "Jane Doe", "ownerGroup": "p12345", "ownerEmail": "jane@example.org", "description": "",
        "datasetName": "my_demo_run", "proposalId": "20.500/prop", "principalInvestigator": "jane@example.org",
        "creationLocation": "/PSI/SLS/TOMCAT2", "type": "raw",
        "scientificMetadata": {"energy": 12.4, "sample": {"description": "", "name": "", "principalInvestigator": ""}}})")
            .arg(m_dir->filePath("data")).toUtf8()).object();
    QCOMPARE(metadata, expected);
    QCOMPARE(scicat::fileListing(snapshot.paths, m_dir->filePath("data")), QStringList({"scan 1", "notes.txt"}));

    // derived data needs an investigator instead, and its scientific metadata is not sent (as in the Widgets GUI)
    draft->setRaw(false);
    draft->setInvestigator(QString());
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    draft->setInvestigator(QStringLiteral("jdoe@example.org"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));
    const QJsonObject derived = scicat::metadataJson(draft->snapshot(), QStringLiteral("/x"));
    QVERIFY(!derived.contains(QStringLiteral("scientificMetadata")));
    QCOMPARE(derived.value("inputDatasets").toArray(), QJsonArray{"N/A"});
}

void BackendTest::failedRunKeepsTheDraft()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    ArchiveDraft *draft = m_backend->archive();
    QFile file(m_dir->filePath(QStringLiteral("one.h5")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    // submitting an empty draft starts nothing and lists what is missing
    QVERIFY(!draft->submit());
    QVERIFY(!draft->run());
    QCOMPARE(draft->problems().size(), 2);

    draft->files()->addPaths({file.fileName()});
    draft->setStep(ArchiveDraft::DetailsStep);
    draft->setDescription(QStringLiteral("kept"));
    QCOMPARE(draft->datasetName(), QStringLiteral("one"));
    QVERIFY(draft->submit());
    IngestRun *run = draft->run();
    QVERIFY(run);
    QVERIFY(!draft->submit()); // one run at a time
    QTRY_COMPARE_WITH_TIMEOUT(run->state(), IngestRun::Failed, 10000); // the CLI path does not exist
    QVERIFY2(run->log().contains(QStringLiteral("--token <hidden>")), qPrintable(run->log()));
    QVERIFY(!run->log().contains(QStringLiteral("good")));
    QVERIFY(run->resultText().contains(QStringLiteral("could not be started")));
    QCOMPARE(m_backend->activity()->index(0).data(ActivityModel::StateRole).toInt(), int(ActivityModel::Failed));

    // the Widgets GUI empties the file list here; this keeps everything for a second try
    QCOMPARE(draft->files()->count(), 1);
    QCOMPARE(draft->datasetName(), QStringLiteral("one"));
    QCOMPARE(draft->description(), QStringLiteral("kept"));

    // what scicat-cli would have been given
    QDir ingestion(m_dir->filePath(QStringLiteral("log/ingestion")));
    const QStringList folders = ingestion.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QCOMPARE(folders.size(), 1);
    QVERIFY(folders.first().startsWith(QStringLiteral("one")));
    QVERIFY(QFile::exists(ingestion.filePath(folders.first() + QStringLiteral("/metadata.json"))));
    QVERIFY(QFile::exists(ingestion.filePath(folders.first() + QStringLiteral("/filelisting.txt"))));
    QVERIFY(QFile::exists(ingestion.filePath(folders.first() + QStringLiteral("/ingestion.log"))));

    // "system calls off" runs nothing and says so
    m_backend->settings()->setDisableSystemCalls(true);
    QVERIFY(draft->submit());
    QTRY_COMPARE(draft->run()->state(), IngestRun::Succeeded);
    QVERIFY(draft->run()->resultText().contains(QStringLiteral("switched off")));

    draft->startNew(true);
    QCOMPARE(draft->files()->count(), 0);
    QCOMPARE(draft->datasetName(), QString());
    QCOMPARE(draft->description(), QStringLiteral("kept"));
    draft->startNew(false);
    QCOMPARE(draft->description(), QString());
}

void BackendTest::metadataModelEditing()
{
    MetadataModel model;
    model.resetToTemplate();
    // sample, then its three children in key order
    QCOMPARE(model.count(), 4);
    QCOMPARE(model.fieldCount(), 3);
    QVERIFY(!model.hasProblems());
    QCOMPARE(model.index(0).data(MetadataModel::KeyRole).toString(), QStringLiteral("sample"));
    QVERIFY(model.index(0).data(MetadataModel::ContainerRole).toBool());
    QCOMPARE(model.index(2).data(MetadataModel::DepthRole).toInt(), 1);

    model.toggle(0);
    QCOMPARE(model.count(), 1);
    model.toggle(0);
    QCOMPARE(model.count(), 4);

    const int group = model.addGroup(-1);
    QCOMPARE(group, 4);
    model.setKey(group, QStringLiteral("beam"));
    const int field = model.addField(group);
    QCOMPARE(field, 5);
    QVERIFY(model.hasProblems()); // no name yet
    model.setKey(field, QStringLiteral("energy"));
    model.setKind(field, MetadataModel::Number);
    QVERIFY(!model.index(field).data(MetadataModel::ValueProblemRole).toString().isEmpty());
    model.setValue(field, QStringLiteral("1e-3"));
    QVERIFY(!model.hasProblems());
    const int flag = model.addField(group);
    model.setKey(flag, QStringLiteral("energy"));
    QVERIFY(!model.index(flag).data(MetadataModel::KeyProblemRole).toString().isEmpty()); // used twice
    model.setKey(flag, QStringLiteral("focused"));
    model.setKind(flag, MetadataModel::YesNo);
    model.setValue(flag, QStringLiteral("true"));

    const QJsonObject json = model.toJson();
    QCOMPARE(json.value("beam").toObject(), QJsonObject({{"energy", 0.001}, {"focused", true}}));

    // the first problem is found also inside a closed group, which is opened for it
    QCOMPARE(model.revealFirstProblem(), -1);
    model.setValue(field, QStringLiteral("fast"));
    model.toggle(group);
    QCOMPARE(model.count(), 5);
    QCOMPARE(model.revealFirstProblem(), field);
    QCOMPARE(model.count(), 7);
    QVERIFY(model.index(group).data(MetadataModel::ExpandedRole).toBool());
    model.setValue(field, QStringLiteral("1e-3"));
    QVERIFY(json.value("sample").isObject());

    model.removeAt(group); // the group goes with its two entries
    QCOMPARE(model.count(), 4);
    QVERIFY(!model.toJson().contains(QStringLiteral("beam")));
    model.clear();
    QVERIFY(model.empty());
}

void BackendTest::metadataImportExport()
{
    MetadataModel model;
    const QString file = m_dir->filePath(QStringLiteral("meta.json"));
    const QJsonObject metadata = QJsonDocument::fromJson(R"({"beam":{"energy":{"value":12.41,"unit":"keV"}},"cooled":true,"runs":[1,2,"x"],"note":"n"})").object();
    const auto write = [&file](const QJsonObject &object) {
        QFile out(file);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(QJsonDocument(object).toJson());
    };
    // a plain object, the wrapped object, and the Widgets GUI's export shape all load to the same tree, types intact
    for (const QJsonObject &shape : {metadata, QJsonObject{{"scientificMetadata", metadata}}, QJsonObject{{"scientificMetadata", QJsonArray{metadata}}}}) {
        write(shape);
        QCOMPARE(model.importFrom(QUrl::fromLocalFile(file)), QString());
        QCOMPARE(model.toJson(), metadata);
    }
    QCOMPARE(model.exportTo(QUrl::fromLocalFile(file)), QString());
    QFile in(file);
    QVERIFY(in.open(QIODevice::ReadOnly));
    QCOMPARE(QJsonDocument::fromJson(in.readAll()).object(), QJsonObject({{"scientificMetadata", metadata}}));

    QFile broken(file);
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("{not json");
    broken.close();
    QVERIFY(model.importFrom(QUrl::fromLocalFile(file)).contains(QStringLiteral("not valid JSON")));
    QCOMPARE(model.toJson(), metadata); // a failed import changes nothing

    // an import can be undone: the entries come back in the order they had, not sorted
    MetadataModel edited;
    const int zebra = edited.addField(-1);
    edited.setKey(zebra, QStringLiteral("zebra"));
    const int apple = edited.addField(-1);
    edited.setKey(apple, QStringLiteral("apple"));
    write(metadata);
    QCOMPARE(edited.importFrom(QUrl::fromLocalFile(file)), QString());
    QCOMPARE(edited.toJson(), metadata);
    edited.undoImport();
    QCOMPARE(edited.count(), 2);
    QCOMPARE(edited.index(0).data(MetadataModel::KeyRole).toString(), QStringLiteral("zebra"));
    QCOMPARE(edited.index(1).data(MetadataModel::KeyRole).toString(), QStringLiteral("apple"));
    edited.undoImport(); // nothing left to undo
    QCOMPARE(edited.count(), 2);
    QVERIFY(!model.importFrom(QUrl::fromLocalFile(m_dir->filePath("missing.json"))).isEmpty());
}

void BackendTest::signOutForgetsEverything()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    m_backend->retrieve()->ensureLoaded();
    QTRY_COMPARE(m_backend->retrieve()->loadState(), RetrieveController::Loaded);
    m_backend->archive()->setDescription(QStringLiteral("draft"));
    m_backend->session()->signOut();
    QCOMPARE(m_backend->session()->state(), Session::SignedOut);
    QVERIFY(m_backend->session()->token().isEmpty());
    QCOMPARE(m_backend->workspace()->state(), Workspace::NoGroup);
    QCOMPARE(m_backend->retrieve()->totalCount(), 0);
    QCOMPARE(m_backend->retrieve()->loadState(), RetrieveController::Idle);
    QCOMPARE(m_backend->archive()->description(), QString());
    QCOMPARE(m_backend->activity()->count(), 0);

    // changing the server while signed in signs out, and the choice is remembered
    signIn();
    m_backend->session()->setServerId(1);
    QCOMPARE(m_backend->session()->state(), Session::SignedOut);
    QCOMPARE(m_backend->settings()->store()->value(QStringLiteral("serverID")).toInt(), 1);
    // the last pgroup comes back after signing in again
    signIn();
    QTRY_COMPARE(m_backend->workspace()->pgroup(), QStringLiteral("p12345"));
}

// A second reload while the first is still asking for the jobs used to leave the list "loading" for good.
void BackendTest::reloadTwiceStillLoads()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    retrieve->reload();
    retrieve->reload();
    retrieve->reload();
    QCOMPARE(retrieve->loadState(), RetrieveController::Loading);
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    QCOMPARE(retrieve->totalCount(), 4);
    // and a jobs-only refresh that overlaps a reload does not get in its way either
    retrieve->refreshJobs();
    retrieve->reload();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
}

void BackendTest::ticksAndCurrentDatasetFollowTheData()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    RetrieveController *retrieve = m_backend->retrieve();
    DatasetFilterModel *list = retrieve->datasets();
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);

    // ticking everything is one notification, not one per row
    QSignalSpy selection(retrieve, &RetrieveController::selectionChanged);
    retrieve->setListedSelected(true);
    QCOMPARE(selection.count(), 1);
    QCOMPARE(retrieve->selectedCount(), 4);
    QCOMPARE(retrieve->filteredSelection(), 2);
    QCOMPARE(retrieve->copyableCount(), 1);     // beta is ready
    QCOMPARE(retrieve->requestableCount(), 3);  // alpha, gamma, and beta again
    QVERIFY(retrieve->selectionHint().contains(QStringLiteral("1 not in the archive yet")));
    retrieve->clearSelection();
    QCOMPARE(selection.count(), 2);
    QCOMPARE(retrieve->selectedCount(), 0);
    QCOMPARE(retrieve->requestableCount(), 0);

    // the current dataset is an ID: its row follows sorting and filtering
    retrieve->setCurrentPid(QStringLiteral("pid/b"));
    QCOMPARE(retrieve->currentRow(), 2); // newest first: delta, gamma, beta, alpha
    list->sortBy(DatasetFilterModel::ByName);
    QCOMPARE(retrieve->currentRow(), 1); // alpha, beta, delta, gamma
    list->setSearch(QStringLiteral("gam"));
    QCOMPARE(retrieve->currentRow(), -1); // filtered out; the panel keeps it
    QCOMPARE(retrieve->currentPid(), QStringLiteral("pid/b"));
    list->clearFilters();
    QCOMPARE(retrieve->currentRow(), 1);

    // ticks and the current dataset survive a reload, since they belong to datasets and not to rows
    retrieve->toggle(list->rowOfPid(QStringLiteral("pid/a")));
    retrieve->toggle(list->rowOfPid(QStringLiteral("pid/d")));
    QCOMPARE(retrieve->selectedCount(), 2);
    retrieve->reload();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    QCOMPARE(retrieve->selectedCount(), 2);
    QVERIFY(list->data(list->index(list->rowOfPid(QStringLiteral("pid/a")), 0), DatasetListModel::SelectedRole).toBool());
    QCOMPARE(retrieve->currentPid(), QStringLiteral("pid/b"));
    QCOMPARE(retrieve->requestableCount(), 1); // alpha; delta is not in the archive

    // another pgroup starts clean
    openGroup(QStringLiteral("p22222"));
    QCOMPARE(retrieve->selectedCount(), 0);
    QCOMPARE(retrieve->currentPid(), QString());
    QCOMPARE(list->search(), QString());
}

void BackendTest::requestsAreKnownWithoutTheRetrievePage()
{
    RetrieveController *retrieve = m_backend->retrieve();
    QSignalSpy jobs(retrieve, &RetrieveController::jobsChanged);
    signIn();
    // nobody opened Retrieve or chose a pgroup: the Activity page asks when it is opened, and lists the user's two requests
    QCOMPARE(m_backend->activity()->count(), 0);
    retrieve->refreshJobs();
    QTRY_COMPARE(m_backend->activity()->count(), 2);
    QCOMPARE(jobs.count(), 1);
    QCOMPARE(m_backend->activity()->index(0).data(ActivityModel::StateRole).toInt(), int(ActivityModel::Failed));
    const QString firstId = m_backend->activity()->index(0).data(ActivityModel::IdRole).toString();
    QVERIFY(!firstId.isEmpty());

    // asking again and getting the same answer changes nothing: no signal, no reset of the list
    QSignalSpy resets(m_backend->activity(), &QAbstractItemModel::modelReset);
    const int requests = int(m_stub->requests.size());
    retrieve->refreshJobs();
    QTRY_VERIFY(int(m_stub->requests.size()) > requests);
    QTest::qWait(50);
    QCOMPARE(jobs.count(), 1);
    QCOMPARE(resets.count(), 0);

    // they stay when the pgroup changes, and a new one is added on top without rebuilding the others
    openGroup(QStringLiteral("p12345"));
    QCOMPARE(m_backend->activity()->count(), 2);
    retrieve->ensureLoaded();
    QTRY_COMPARE(retrieve->loadState(), RetrieveController::Loaded);
    retrieve->toggle(retrieve->datasets()->rowOfPid(QStringLiteral("pid/a")));
    retrieve->requestSelected();
    QTRY_COMPARE(m_backend->activity()->count(), 3);
    QCOMPARE(resets.count(), 0);
    QCOMPARE(m_backend->activity()->index(1).data(ActivityModel::IdRole).toString(), firstId);
    QCOMPARE(m_backend->activity()->index(0).data(ActivityModel::TitleRole).toString(), QStringLiteral("Request alpha"));
}

void BackendTest::signOutWhileARunIsActive()
{
    // a stand-in for scicat-cli that keeps running
    const QString cli = m_dir->filePath(QStringLiteral("slow-cli"));
    QFile script(cli);
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write("#!/bin/sh\nsleep 30\n");
    script.close();
    QVERIFY(script.setPermissions(script.permissions() | QFileDevice::ExeOwner));

    signIn();
    openGroup(QStringLiteral("p12345"));
    // after signing in: at sign-in the application looks for scicat-cli itself and stores what it finds
    m_backend->settings()->setCliPath(cli);
    ArchiveDraft *draft = m_backend->archive();
    QFile file(m_dir->filePath(QStringLiteral("one.h5")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    draft->files()->addPaths({file.fileName()});
    draft->setStep(ArchiveDraft::DetailsStep);
    draft->setDescription(QStringLiteral("belongs to jdoe"));
    QVERIFY(draft->submit());
    QPointer<IngestRun> run = draft->run();
    QTRY_COMPARE(run->state(), IngestRun::Running);
    QCOMPARE(m_backend->activity()->runningCount(), 1);

    m_backend->session()->signOut();
    // nothing of the dataset is left for whoever signs in next, although the process is still being stopped
    QVERIFY(!draft->run());
    QCOMPARE(draft->files()->count(), 0);
    QCOMPARE(draft->description(), QString());
    QCOMPARE(draft->datasetName(), QString());
    QCOMPARE(m_backend->activity()->count(), 0);
    // the run ends on its own and is then deleted
    QTRY_VERIFY_WITH_TIMEOUT(run.isNull(), 8000);
}

// The dialog for input datasets: ticks and a search from last time are gone, and a dataset registered since the last
// look is listed.
void BackendTest::pickerStartsCleanAndCurrent()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    ArchiveDraft *draft = m_backend->archive();
    draft->openPicker();
    QCOMPARE(draft->pickerState(), ArchiveDraft::PickerLoading);
    QTRY_COMPARE(draft->pickerState(), ArchiveDraft::PickerReady);
    QCOMPARE(draft->picker()->count(), 4);
    draft->picker()->setSearch(QStringLiteral("alp"));
    QTRY_COMPARE(draft->picker()->count(), 1);
    draft->togglePicked(0);
    QCOMPARE(draft->pickedCount(), 1);
    // closed without adding, then opened again
    QSignalSpy startedNew(draft, &ArchiveDraft::startedNew);
    m_catalogue->extraDataset = true;
    const int before = m_catalogue->datasetListRequests;
    draft->openPicker();
    QCOMPARE(draft->pickedCount(), 0);
    QCOMPARE(draft->picker()->search(), QString());
    QTRY_COMPARE(draft->pickerState(), ArchiveDraft::PickerReady);
    QVERIFY(m_catalogue->datasetListRequests > before);
    QCOMPARE(draft->picker()->count(), 2);
    QVERIFY(draft->picker()->rowOfPid(QStringLiteral("pid/e")) >= 0);
    // opening it twice in a row asks once
    const int again = m_catalogue->datasetListRequests;
    draft->openPicker();
    draft->openPicker();
    QTRY_COMPARE(draft->pickerState(), ArchiveDraft::PickerReady);
    QCOMPARE(m_catalogue->datasetListRequests, again + 1);
    // a new dataset says so, for what a page keeps beside the draft
    draft->startNew(false);
    QCOMPARE(startedNew.count(), 1);
}

void BackendTest::undoOfAResetEndsWithTheNextChange()
{
    Settings *settings = m_backend->settings();
    settings->setSaveLog(false);
    settings->setMetadataFileName(QStringLiteral("mine.json"));
    QVERIFY(!settings->canUndoReset());
    settings->resetToDefaults();
    QVERIFY(settings->canUndoReset());
    QCOMPARE(settings->metadataFileName(), QStringLiteral("metadata.json"));
    settings->undoReset();
    QVERIFY(!settings->canUndoReset());
    QCOMPARE(settings->metadataFileName(), QStringLiteral("mine.json"));
    QVERIFY(!settings->saveLog());

    // a change made after a reset is kept: "Undo" is no longer offered, as it would take that change back too
    settings->resetToDefaults();
    QVERIFY(settings->canUndoReset());
    settings->setShowCommand(!settings->showCommand());
    QVERIFY(!settings->canUndoReset());
    settings->undoReset();
    QCOMPARE(settings->metadataFileName(), QStringLiteral("metadata.json"));
}

// "Register by default" and "Send to tape by default" are the starting values of the two boxes of the review step.
void BackendTest::sendOptionsFollowTheSettingsUntilTouched()
{
    Settings *settings = m_backend->settings();
    ArchiveDraft *draft = m_backend->archive();
    QVERIFY(draft->ingest());
    QVERIFY(draft->autoArchive());
    // changed in the settings before the review step was looked at: the dataset in the making follows
    settings->setEnableIngest(false);
    settings->setEnableArchive(false);
    QVERIFY(!draft->ingest());
    QVERIFY(!draft->autoArchive());
    // set for this dataset: the settings no longer reach it
    draft->setIngest(true);
    settings->setEnableIngest(true);
    settings->setEnableIngest(false);
    QVERIFY(draft->ingest());
    // the next dataset starts from the settings again
    draft->startNew(false);
    QVERIFY(!draft->ingest());
}

// The Activity page asks for the user's retrieve requests when it is opened, unless Retrieve has asked already.
void BackendTest::activityAsksForTheRequestsOnce()
{
    const auto asked = [this] {
        int count = 0;
        for (const HttpStub::Request &request : std::as_const(m_stub->requests))
            count += request.path == QLatin1String("/api/v4/jobs") && request.method == QLatin1String("GET");
        return count;
    };
    RetrieveController *retrieve = m_backend->retrieve();
    signIn();
    // an answer that does not come is not an answer: the page asks again the next time it is opened
    m_catalogue->failJobs = true;
    retrieve->ensureJobsLoaded();
    QTRY_COMPARE(asked(), 1);
    QTest::qWait(50);
    QCOMPARE(m_backend->activity()->count(), 0);
    m_catalogue->failJobs = false;
    retrieve->ensureJobsLoaded();
    QTRY_COMPARE(m_backend->activity()->count(), 2);
    QCOMPARE(asked(), 2);
    retrieve->ensureJobsLoaded();
    QTest::qWait(50);
    QCOMPARE(asked(), 2);
    // signing in again asks again: the requests are the user's
    m_backend->session()->signOut();
    signIn();
    QCOMPARE(m_backend->activity()->count(), 0);
    retrieve->ensureJobsLoaded();
    QTRY_COMPARE(asked(), 3);
}

// User::updateUserInfo() of the Widgets GUI: the address of the identity profile, else the one of auth/whoami.
void BackendTest::emailComesFromTheIdentityProfile()
{
    m_catalogue->identityEmail = QStringLiteral("john.doe@psi.example");
    signIn();
    QCOMPARE(m_catalogue->identityRequests, 1);
    QCOMPARE(m_backend->session()->email(), QStringLiteral("john.doe@psi.example"));
    QCOMPARE(m_backend->session()->username(), QStringLiteral("jdoe"));
    // it is what a retrieve request names as contact, and what derived data gets as investigator
    openGroup(QStringLiteral("p12345"));
    m_backend->archive()->setStep(ArchiveDraft::DetailsStep);
    QCOMPARE(m_backend->archive()->investigator(), QStringLiteral("john.doe@psi.example"));

    // without a profile the sign-in still succeeds, with the address of auth/whoami
    m_backend->session()->signOut();
    m_catalogue->identityEmail.clear();
    signIn();
    QCOMPARE(m_catalogue->identityRequests, 2);
    QCOMPARE(m_backend->session()->email(), QStringLiteral("jdoe@example.org"));
}

// The owner fields of a dataset: Dialog::derivedFieldsChanged(), Dialog::checkMetadata() and the getters of
// Experiment in the Widgets GUI.
void BackendTest::ownerFieldsFollowTheProposalUntilTyped()
{
    signIn();
    openGroup(QStringLiteral("p12345"));
    ArchiveDraft *draft = m_backend->archive();
    QFile file(m_dir->filePath(QStringLiteral("one.h5")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    draft->files()->addPaths({file.fileName()});
    draft->setStep(ArchiveDraft::DetailsStep);

    // as the proposal says
    QCOMPARE(draft->owner(), QStringLiteral("Jane Doe"));
    QCOMPARE(draft->ownerEmail(), QStringLiteral("jane@example.org"));
    QCOMPARE(draft->ownerGroup(), QStringLiteral("p12345"));
    QCOMPARE(draft->principalInvestigator(), QStringLiteral("jane@example.org"));
    QCOMPARE(draft->proposalId(), QStringLiteral("20.500/prop"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));

    // typed over
    QSignalSpy changed(draft, &ArchiveDraft::ownershipChanged);
    draft->setOwner(QStringLiteral("Ada Lovelace"));
    draft->setOwnerEmail(QStringLiteral(" ada@example.org "));
    draft->setOwnerGroup(QStringLiteral("p22222"));
    draft->setPrincipalInvestigator(QStringLiteral("Charles Babbage"));
    draft->setProposalId(QStringLiteral(" 20.500/other "));
    QCOMPARE(changed.count(), 5);
    scicat::DatasetDraft snapshot = draft->snapshot();
    QCOMPARE(snapshot.owner, QStringLiteral("Ada Lovelace"));
    QCOMPARE(snapshot.ownerEmail, QStringLiteral("ada@example.org"));
    QCOMPARE(snapshot.ownerGroup, QStringLiteral("p22222"));
    QCOMPARE(snapshot.principalInvestigator, QStringLiteral("Charles Babbage"));
    QCOMPARE(snapshot.proposalId, QStringLiteral("20.500/other"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));
    // the lists of the pgroup do not move with the owner group
    QCOMPARE(m_backend->workspace()->pgroup(), QStringLiteral("p12345"));

    // an emptied email or principal investigator goes back to the proposal's; an emptied proposal ID stays empty
    draft->setOwnerEmail(QString());
    draft->setPrincipalInvestigator(QStringLiteral("  "));
    draft->setProposalId(QString());
    QCOMPARE(draft->ownerEmail(), QStringLiteral("jane@example.org"));
    QCOMPARE(draft->principalInvestigator(), QStringLiteral("jane@example.org"));
    QCOMPARE(draft->proposalId(), QString());
    QVERIFY(!scicat::metadataJson(draft->snapshot(), QStringLiteral("/x")).contains(QStringLiteral("proposalId")));

    // the three checks
    draft->setOwnerEmail(QStringLiteral("not an address"));
    QVERIFY(!draft->ownerEmailProblem().isEmpty());
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    draft->setOwnerEmail(QStringLiteral("ada@example.org"));
    QCOMPARE(draft->ownerEmailProblem(), QString());
    draft->setOwner(QStringLiteral(" "));
    QVERIFY(!draft->ownerProblem().isEmpty());
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    draft->setOwner(QStringLiteral("Ada Lovelace"));
    draft->setOwnerGroup(QStringLiteral("p99999"));
    QVERIFY(!draft->ownerGroupProblem().isEmpty());
    QVERIFY(!draft->checkStep(ArchiveDraft::DetailsStep));
    QVERIFY(!draft->submit());
    draft->setOwnerGroup(QStringLiteral("p12345"));
    QVERIFY(draft->checkStep(ArchiveDraft::DetailsStep));

    // a dataset with the same details keeps what was typed, a fresh one and another pgroup start from the proposal
    draft->startNew(true);
    QCOMPARE(draft->owner(), QStringLiteral("Ada Lovelace"));
    draft->startNew(false);
    QCOMPARE(draft->owner(), QStringLiteral("Jane Doe"));
    QCOMPARE(draft->ownerEmail(), QStringLiteral("jane@example.org"));
    QCOMPARE(draft->proposalId(), QStringLiteral("20.500/prop"));
    draft->setOwner(QStringLiteral("Ada Lovelace"));
    draft->setOwnerGroup(QStringLiteral("p12345"));
    openGroup(QStringLiteral("p22222"));
    QCOMPARE(draft->owner(), QStringLiteral(" "));
    QCOMPARE(draft->ownerGroup(), QStringLiteral("p22222"));
    QCOMPARE(draft->proposalId(), QString());
}

// #22 of the Widgets GUI: the backend may hand out a masked address for the PI. It counts as "there is one", is not
// used as an address, and the dataset falls back to the user's own address and the PI's name.
void BackendTest::maskedPiEmailFallsBackToTheUser()
{
    m_catalogue->piEmail = QStringLiteral("*****");
    signIn();
    openGroup(QStringLiteral("p12345"));
    Workspace *workspace = m_backend->workspace();
    QVERIFY(workspace->piComplete());
    QVERIFY(!workspace->piEditable());
    ArchiveDraft *draft = m_backend->archive();
    QCOMPARE(draft->ownerEmail(), QStringLiteral("jdoe@example.org"));
    QCOMPARE(draft->principalInvestigator(), QStringLiteral("Jane Doe"));
    const QJsonObject metadata = scicat::metadataJson(draft->snapshot(), QStringLiteral("/x"));
    QCOMPARE(metadata.value("ownerEmail").toString(), QStringLiteral("jdoe@example.org"));
    QCOMPARE(metadata.value("principalInvestigator").toString(), QStringLiteral("Jane Doe"));
    // "investigator is PI" names the same person
    m_backend->settings()->setInvestigatorIsPI(true);
    draft->setStep(ArchiveDraft::DetailsStep);
    QCOMPARE(draft->investigator(), QStringLiteral("Jane Doe"));
}

void BackendTest::typedPiEmailHasToBeAnAddress()
{
    signIn();
    openGroup(QStringLiteral("p22222")); // no proposal: the PI is typed in
    Workspace *workspace = m_backend->workspace();
    QVERIFY(workspace->piEditable());
    workspace->setPiFirstName(QStringLiteral("Ada"));
    workspace->setPiLastName(QStringLiteral("Lovelace"));
    workspace->setPiEmail(QStringLiteral("ada"));
    QVERIFY(!workspace->piEmailValid());
    QVERIFY(!workspace->piComplete());
    QVERIFY(!workspace->archiveReady());
    workspace->setPiEmail(QStringLiteral("ada@example.org"));
    QVERIFY(workspace->piEmailValid());
    QVERIFY(workspace->piComplete());
    QCOMPARE(m_backend->archive()->ownerEmail(), QStringLiteral("ada@example.org"));
    QCOMPARE(m_backend->archive()->owner(), QStringLiteral("Ada Lovelace"));
}

QTEST_GUILESS_MAIN(BackendTest)
#include "tst_backend.moc"
