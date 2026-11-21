#include "scenarioplayer.h"

#include "backend.h"
#include "copyqueue.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <algorithm>
#include <map>

namespace {

// A step that could not be done as the scenario says. Its text becomes the result "error: ...", which matches no
// expectation.
struct StepError
{
    QString message;
};

// What the application has no control for, because it does the thing differently on purpose. The scenario goes on.
const QString unavailable = QStringLiteral("unavailable");

QString textOf(const QObject *object)
{
    return object ? object->property("text").toString() : QString();
}

QStringList stringList(const QJsonValue &value)
{
    QStringList list;
    for (const QJsonValue &entry : value.toArray())
        list << entry.toString();
    return list;
}

// dialogs the player opens itself to work in; the others are questions and messages, which are written down
bool isTool(const QString &dialog)
{
    return dialog == QLatin1String("settingsDialog") || dialog == QLatin1String("pickerDialog");
}

} // namespace

ScenarioPlayer::ScenarioPlayer(QQuickWindow *window, Backend *backend, const QString &settingsFile, QObject *parent)
    : QObject(parent)
    , m_ui(window)
    , m_backend(backend)
    , m_settingsFile(settingsFile)
{
}

// ---------------------------------------------------------------------------------------------------------------
// settings: the scenario gives and reads back a file of the Widgets GUI

void ScenarioPlayer::importSettings(const QString &given, const QString &own)
{
    QFile::remove(own);
    if (!QFileInfo::exists(given))
        return;
    QSettings from(given, QSettings::IniFormat);
    QSettings to(own, QSettings::IniFormat);
    const QStringList keys = from.allKeys();
    for (const QString &key : keys)
        to.setValue(key, from.value(key));
    // The two applications count the versions of their settings separately: 0.x is the current one of the Widgets
    // GUI, 1.0 the current one here. Any other major version stays another major version.
    const QVersionNumber version = QVersionNumber::fromString(from.value(QStringLiteral("version")).toString());
    if (!version.isNull()) {
        to.setValue(QStringLiteral("version"), version.majorVersion() == 0 ? QStringLiteral("1.0")
                        : QVersionNumber(version.majorVersion() + 1, version.minorVersion()).toString());
    }
    to.sync();
}

// Every preference with the value in effect, as the Widgets GUI keeps them in its file (it writes the defaults out),
// and whatever else is stored. Window sizes and the like, under "ui/", are this application's own business.
void ScenarioPlayer::exportSettings() const
{
    const scicat::AppSettings *store = m_backend->settings()->store();
    QFile::remove(m_settingsFile);
    QSettings out(m_settingsFile, QSettings::IniFormat);
    const QVariantMap preferences = store->snapshot();
    for (auto it = preferences.constBegin(); it != preferences.constEnd(); ++it)
        out.setValue(it.key(), it.value());
    const QStringList stored = store->keys();
    for (const QString &key : stored) {
        if (!key.startsWith(QLatin1String("ui/")) && !preferences.contains(key))
            out.setValue(key, store->value(key));
    }
    out.sync();
}

// ---------------------------------------------------------------------------------------------------------------
// running

void ScenarioPlayer::start()
{
    QFile file(qEnvironmentVariable("SCICAT_TEST_SCENARIO"));
    if (!file.open(QIODevice::ReadOnly))
        qFatal("scenario driver: cannot read SCICAT_TEST_SCENARIO");
    const QJsonDocument scenario = QJsonDocument::fromJson(file.readAll());
    if (!scenario.isObject())
        qFatal("scenario driver: SCICAT_TEST_SCENARIO is not a scenario");
    m_scenario = scenario.object();
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        m_gone = true;
        save();
    });
    QTimer::singleShot(0, this, &ScenarioPlayer::run);
}

void ScenarioPlayer::run()
{
    pause(100);
    noteDialogs();
    m_messagesBefore = messagesShown();
    const QJsonArray steps = m_scenario.value(QLatin1String("steps")).toArray();
    for (const QJsonValue &value : steps) {
        const QJsonObject step = value.toObject();
        m_answer = step.value(QLatin1String("answer")).toString(QStringLiteral("yes"));
        QString result;
        if (m_gone) {
            result = QStringLiteral("error: the application has quit");
        } else {
            try {
                result = runStep(step);
            } catch (const StepError &error) {
                result = QStringLiteral("error: ") + error.message;
            }
        }
        if (!m_gone) {
            m_ui.settle();
            noteDialogs();
            noteMessages();
        }
        m_steps.append(QJsonObject{{QStringLiteral("do"), step.value(QLatin1String("do"))}, {QStringLiteral("result"), result}});
        save();
    }
    if (!m_gone)
        QCoreApplication::exit(0);
}

void ScenarioPlayer::save() const
{
    exportSettings();
    QFile out(qEnvironmentVariable("SCICAT_TEST_OUT"));
    if (!out.open(QIODevice::WriteOnly))
        return;
    const QJsonObject messages{{QStringLiteral("dialogs"), m_dialogs}, {QStringLiteral("banners"), m_banners}};
    out.write(QJsonDocument(QJsonObject{{QStringLiteral("driver"), QStringLiteral("qml")},
                                        {QStringLiteral("steps"), m_steps},
                                        {QStringLiteral("messages"), messages},
                                        {QStringLiteral("snapshots"), m_snapshots}})
                  .toJson());
}

QString ScenarioPlayer::runStep(const QJsonObject &step)
{
    static const std::map<QString, QString (ScenarioPlayer::*)(const QJsonObject &)> steps{
        {QStringLiteral("signIn"), &ScenarioPlayer::signIn},
        {QStringLiteral("changeSettings"), &ScenarioPlayer::changeSettings},
        {QStringLiteral("choosePgroup"), &ScenarioPlayer::choosePgroup},
        {QStringLiteral("enterPI"), &ScenarioPlayer::enterPI},
        {QStringLiteral("archive.open"), &ScenarioPlayer::openArchive},
        {QStringLiteral("archive.addFiles"), &ScenarioPlayer::addFiles},
        {QStringLiteral("archive.toMetadata"), &ScenarioPlayer::toMetadata},
        {QStringLiteral("archive.setMetadata"), &ScenarioPlayer::setMetadata},
        {QStringLiteral("archive.submit"), &ScenarioPlayer::submit},
        {QStringLiteral("archive.restart"), &ScenarioPlayer::restart},
        {QStringLiteral("retrieve.open"), &ScenarioPlayer::openRetrieve},
        {QStringLiteral("retrieve.select"), &ScenarioPlayer::selectDatasets},
        {QStringLiteral("retrieve.request"), &ScenarioPlayer::requestRetrieve},
        {QStringLiteral("retrieve.copy"), &ScenarioPlayer::copyRetrieved},
        {QStringLiteral("jobs.open"), &ScenarioPlayer::openJobs},
        {QStringLiteral("jobs.refresh"), &ScenarioPlayer::refreshJobs},
        {QStringLiteral("quit"), &ScenarioPlayer::quit},
        {QStringLiteral("snapshot"), &ScenarioPlayer::snapshot},
    };
    const QString name = step.value(QLatin1String("do")).toString();
    if (name == QLatin1String("waitForFiles")) {
        const int timeout = step.value(QLatin1String("timeout")).toInt(20000);
        const QStringList paths = stringList(step.value(QLatin1String("paths")));
        for (const QString &path : paths)
            waitFor([&path] { return QFileInfo::exists(path); }, timeout, path);
        return QStringLiteral("ok");
    }
    if (name == QLatin1String("removeFile")) {
        QFile::remove(step.value(QLatin1String("path")).toString());
        return QStringLiteral("ok");
    }
    if (name == QLatin1String("wait")) {
        pause(step.value(QLatin1String("ms")).toInt());
        return QStringLiteral("ok");
    }
    const auto found = steps.find(name);
    if (found == steps.end())
        throw StepError{QStringLiteral("unknown step ") + name};
    return (this->*(found->second))(step);
}

// ---------------------------------------------------------------------------------------------------------------
// waiting

void ScenarioPlayer::pause(int ms)
{
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms && !m_gone) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::msleep(2);
    }
    if (!m_gone)
        m_ui.settle();
}

bool ScenarioPlayer::waitUntil(const std::function<bool()> &done, int ms)
{
    QElapsedTimer clock;
    clock.start();
    qint64 settled = 0;
    while (!m_gone) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        if (m_gone)
            break;
        // a frame now and then: what is shown follows the state with the next frame
        if (clock.elapsed() - settled >= 40) {
            m_ui.settle();
            noteDialogs();
            settled = clock.elapsed();
        }
        if (done())
            return true;
        if (clock.elapsed() > ms)
            return false;
        QThread::msleep(2);
    }
    return false;
}

void ScenarioPlayer::waitFor(const std::function<bool()> &done, int ms, const QString &what)
{
    if (!waitUntil(done, ms))
        throw StepError{QStringLiteral("timed out waiting for ") + what};
}

// ---------------------------------------------------------------------------------------------------------------
// what is on the screen

QQuickItem *ScenarioPlayer::maybe(const QString &name, int nth) const
{
    return m_ui.findAll(name).value(nth);
}

QQuickItem *ScenarioPlayer::need(const QString &name, int nth) const
{
    m_ui.settle();
    if (QQuickItem *item = maybe(name, nth))
        return item;
    throw StepError{QStringLiteral("nothing shown is called %1").arg(name)};
}

QQuickItem *ScenarioPlayer::reach(QQuickItem *item) const
{
    if (!item->isEnabled())
        throw StepError{QStringLiteral("%1 is switched off").arg(UiProbe::nameOf(item))};
    QString why;
    if (!m_ui.bringIntoView(item)) {
        m_ui.inView(item, &why);
        throw StepError{QStringLiteral("%1 is not in view: %2").arg(UiProbe::nameOf(item), why)};
    }
    return item;
}

QQuickItem *ScenarioPlayer::reach(const QString &name, int nth) const
{
    return reach(need(name, nth));
}

void ScenarioPlayer::click(const QString &name, int nth) const
{
    m_ui.click(reach(name, nth));
    m_ui.settle();
}

void ScenarioPlayer::typeInto(const QString &name, const QString &text) const
{
    m_ui.typeInto(reach(name), text);
    m_ui.settle();
}

void ScenarioPlayer::key(const QString &name) const
{
    m_ui.key(name);
    m_ui.settle();
}

// Opens the drop-down list of the box and clicks the entry that says "text".
void ScenarioPlayer::chooseEntry(const QString &comboName, const QString &text) const
{
    QQuickItem *combo = reach(comboName);
    if (combo->property("currentText").toString() == text)
        return;
    int index = -1;
    QMetaObject::invokeMethod(combo, "find", Q_RETURN_ARG(int, index), Q_ARG(QString, text));
    if (index < 0)
        throw StepError{QStringLiteral("%1 has no entry %2").arg(comboName, text)};
    m_ui.click(combo);
    m_ui.settle();
    const QObject *popup = combo->property("popup").value<QObject *>();
    QQuickItem *list = popup ? popup->property("contentItem").value<QQuickItem *>() : nullptr;
    if (!list || !popup->property("visible").toBool())
        throw StepError{QStringLiteral("the list of %1 did not open").arg(comboName)};
    // a list only holds the entries in view: bring the wanted one there, as scrolling would
    if (list->inherits("QQuickListView"))
        QMetaObject::invokeMethod(list, "positionViewAtIndex", Q_ARG(int, index), Q_ARG(int, 4 /* ListView.Contain */));
    m_ui.settle();
    const QList<QQuickItem *> entries = m_ui.everything(list);
    for (QQuickItem *entry : entries) {
        if (entry->inherits("QQuickAbstractButton") && textOf(entry) == text) {
            m_ui.click(reach(entry));
            // the list fades out; until it is gone a click next to it only closes it
            QElapsedTimer closing;
            closing.start();
            while (popup->property("visible").toBool() && closing.elapsed() < 3000) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                QThread::msleep(2);
            }
            m_ui.settle();
            return;
        }
    }
    m_ui.key(QStringLiteral("Escape"));
    throw StepError{QStringLiteral("the list of %1 does not show %2").arg(comboName, text)};
}

void ScenarioPlayer::setSwitch(const QString &name, bool on) const
{
    QQuickItem *control = reach(name);
    if (control->property("checked").toBool() == on)
        return;
    m_ui.click(control);
    m_ui.settle();
    if (control->property("checked").toBool() != on)
        throw StepError{QStringLiteral("%1 did not change when clicked").arg(name)};
}

// The dialogs in front of the window. A dialog's item sits in the window's overlay and belongs to the dialog.
QList<QObject *> ScenarioPlayer::openDialogs() const
{
    QList<QObject *> open;
    if (!m_ui.window())
        return open;
    const QList<QQuickItem *> top = m_ui.window()->contentItem()->childItems();
    for (const QQuickItem *layer : top) {
        if (!layer->inherits("QQuickOverlay"))
            continue;
        const QList<QQuickItem *> items = layer->childItems();
        for (const QQuickItem *item : items) {
            QObject *dialog = item->parent();
            if (item->isVisible() && dialog && dialog->inherits("QQuickDialog") && !dialog->objectName().isEmpty())
                open << dialog;
        }
    }
    return open;
}

QObject *ScenarioPlayer::openDialog(const QString &name) const
{
    const QList<QObject *> open = openDialogs();
    for (QObject *dialog : open) {
        if (dialog->objectName() == name)
            return dialog;
    }
    return nullptr;
}

// Each question or message in front of the window, once: which dialog, its title and its first sentence.
void ScenarioPlayer::noteDialogs()
{
    const QList<QObject *> open = openDialogs();
    for (QObject *dialog : open) {
        if (m_dialogsSeen.contains(dialog))
            continue;
        m_dialogsSeen.insert(dialog);
        if (isTool(dialog->objectName()))
            continue;
        QString text;
        const QQuickItem *inside = dialog->property("contentItem").value<QQuickItem *>();
        const QList<QQuickItem *> content = inside ? m_ui.everything(inside) : QList<QQuickItem *>();
        for (const QQuickItem *item : content) {
            if (item->inherits("QQuickText") && !textOf(item).isEmpty()) {
                text = textOf(item);
                break;
            }
        }
        m_dialogs.append(QJsonObject{{QStringLiteral("dialog"), dialog->objectName()},
                                     {QStringLiteral("title"), dialog->property("title").toString()},
                                     {QStringLiteral("text"), text}});
    }
    // one that was closed and comes back is a new question
    for (auto it = m_dialogsSeen.begin(); it != m_dialogsSeen.end();) {
        if (open.contains(*it))
            ++it;
        else
            it = m_dialogsSeen.erase(it);
    }
}

// What the pages say to the user right now: everything that is an alert or a note for a screen reader (messages,
// what is wrong with a field), and the brief message at the foot of the window.
QStringList ScenarioPlayer::messagesShown() const
{
    QStringList messages;
    const QList<QQuickItem *> items = m_ui.everything();
    for (QQuickItem *item : items) {
        QString text;
        if (item->objectName() == QLatin1String("toastText")) {
            text = textOf(item);
        } else if (const auto *accessible = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(item, false))) {
            if (accessible->role() == QAccessible::AlertMessage || accessible->role() == QAccessible::Note)
                text = accessible->name().isEmpty() ? textOf(item) : accessible->name();
        }
        if (!text.isEmpty())
            messages << text;
    }
    return messages;
}

// at the end of a step: what is said now and was not said before it
void ScenarioPlayer::noteMessages()
{
    const QStringList now = messagesShown();
    for (const QString &message : now) {
        if (!m_messagesBefore.contains(message))
            m_banners.append(message);
    }
    m_messagesBefore = now;
}

// ---------------------------------------------------------------------------------------------------------------
// sign in and settings

QString ScenarioPlayer::signIn(const QJsonObject &step)
{
    Session *session = m_backend->session();
    typeInto(QStringLiteral("tokenField"), step.value(QLatin1String("token")).toString());
    click(QStringLiteral("signInButton"));
    waitFor([session] { return session->state() != Session::SigningIn; }, 30000, QStringLiteral("the answer to the sign-in"));
    if (!session->signedIn())
        return QStringLiteral("rejected");
    waitFor([this] { return maybe(QStringLiteral("groupBox")) != nullptr; }, 5000, QStringLiteral("the first page"));
    return QStringLiteral("signedIn");
}

QString ScenarioPlayer::changeSettings(const QJsonObject &step)
{
    static const std::map<QString, QString> switches{
        {QStringLiteral("deleteLog"), QStringLiteral("settingDeleteLog")},
        {QStringLiteral("loadProposalInfo"), QStringLiteral("settingLoadProposal")},
        {QStringLiteral("saveProposalInfo"), QStringLiteral("settingSaveProposal")},
        {QStringLiteral("saveLog"), QStringLiteral("settingSaveLog")},
        {QStringLiteral("enableArchive"), QStringLiteral("settingEnableArchive")},
        {QStringLiteral("enableIngest"), QStringLiteral("settingEnableIngest")},
        {QStringLiteral("usePgroupAsDefaultPath"), QStringLiteral("settingUsePgroupPath")},
        {QStringLiteral("investigatorIsPI"), QStringLiteral("settingInvestigatorIsPI")},
        {QStringLiteral("guessDatasetname"), QStringLiteral("settingGuessName")},
        {QStringLiteral("printIngestion"), QStringLiteral("settingShowCommand")},
        {QStringLiteral("disableSystemCalls"), QStringLiteral("settingDryMode")},
        {QStringLiteral("requestAlreadyRetrievedData"), QStringLiteral("settingRequestRetrieved")},
    };
    static const std::map<QString, QString> fields{
        {QStringLiteral("filenameMetadata"), QStringLiteral("settingMetadataFile")},
        {QStringLiteral("filenameFilelisting"), QStringLiteral("settingListingFile")},
        {QStringLiteral("logPath"), QStringLiteral("settingLogPath")},
    };
    // every change is stored when it is made: there is nothing to cancel
    if (step.value(QLatin1String("cancel")).toBool())
        return unavailable;

    QJsonObject values = step.value(QLatin1String("values")).toObject();
    const bool signedIn = m_backend->session()->signedIn();
    bool allDone = true;
    if (values.contains(QLatin1String("serverID"))) {
        // the server is chosen on the sign-in page; a token belongs to one server
        if (signedIn)
            allDone = false;
        else
            chooseEntry(QStringLiteral("serverBox"), m_backend->session()->serverNames().value(values.value(QLatin1String("serverID")).toInt()));
        values.remove(QLatin1String("serverID"));
    }
    if (values.isEmpty() && !step.value(QLatin1String("reset")).toBool())
        return allDone ? QStringLiteral("ok") : unavailable;

    if (signedIn) {
        click(QStringLiteral("navSettings"));
        need(QStringLiteral("settingsPage"));
    } else {
        click(QStringLiteral("settingsButton"));
        waitFor([this] { return maybe(QStringLiteral("settingsForm")) != nullptr; }, 3000, QStringLiteral("the settings"));
    }
    if (step.value(QLatin1String("reset")).toBool())
        click(QStringLiteral("settingsReset"));
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (const auto found = switches.find(it.key()); found != switches.end()) {
            setSwitch(found->second, it.value().toBool());
        } else if (const auto field = fields.find(it.key()); field != fields.end()) {
            typeInto(field->second, it.value().toString());
            key(QStringLiteral("Return"));
        } else {
            throw StepError{QStringLiteral("unknown setting ") + it.key()};
        }
    }
    if (!signedIn) {
        click(QStringLiteral("settingsDialogClose"));
        waitFor([this] { return openDialog(QStringLiteral("settingsDialog")) == nullptr; }, 3000, QStringLiteral("the settings to close"));
    }
    return allDone ? QStringLiteral("ok") : unavailable;
}

// ---------------------------------------------------------------------------------------------------------------
// pgroup

QString ScenarioPlayer::choosePgroup(const QJsonObject &step)
{
    if (!maybe(QStringLiteral("groupBox")))
        click(QStringLiteral("navOverview"));
    typeInto(QStringLiteral("groupBox"), step.value(QLatin1String("pgroup")).toString());
    key(QStringLiteral("Return"));
    const Workspace *workspace = m_backend->workspace();
    waitFor([workspace] { return workspace->state() != Workspace::Loading; }, 30000, QStringLiteral("the proposal"));
    m_ui.settle();
    return QStringLiteral("ok");
}

QString ScenarioPlayer::enterPI(const QJsonObject &step)
{
    if (!maybe(QStringLiteral("groupBox")))
        click(QStringLiteral("navOverview"));
    static const std::map<QString, QString> fields{{QStringLiteral("firstname"), QStringLiteral("piFirstName")},
                                                    {QStringLiteral("lastname"), QStringLiteral("piLastName")},
                                                    {QStringLiteral("email"), QStringLiteral("piEmail")}};
    for (const auto &[key, field] : fields) {
        if (step.contains(key))
            typeInto(field, step.value(key).toString());
    }
    return QStringLiteral("ok");
}

// ---------------------------------------------------------------------------------------------------------------
// archive

// From the Overview a person takes the big "Archive data" tile, from anywhere else the entry in the side bar.
QString ScenarioPlayer::openArchive(const QJsonObject &)
{
    if (maybe(QStringLiteral("archiveTile")))
        click(QStringLiteral("archiveTile"));
    else
        click(QStringLiteral("navArchive"));
    pause(50);
    const bool form = maybe(QStringLiteral("archiveStepper")) || maybe(QStringLiteral("runTitle")) || maybe(QStringLiteral("runResult"));
    return form ? QStringLiteral("ok") : QStringLiteral("blocked");
}

QString ScenarioPlayer::addFiles(const QJsonObject &step)
{
    const QDir dir(step.value(QLatin1String("dir")).toString());
    QStringList paths;
    const QStringList names = stringList(step.value(QLatin1String("names")));
    for (const QString &name : names)
        paths << dir.filePath(name);
    const FileEntryModel *files = m_backend->archive()->files();
    const int before = files->count();
    m_ui.drop(reach(QStringLiteral("fileDropArea")), paths);
    waitFor([files, before, &paths] { return files->count() == before + paths.size() && !files->scanning(); }, 20000,
            QStringLiteral("the files to be counted"));
    return QStringLiteral("ok");
}

QString ScenarioPlayer::toMetadata(const QJsonObject &)
{
    const ArchiveDraft *draft = m_backend->archive();
    if (draft->step() != ArchiveDraft::FilesStep || !maybe(QStringLiteral("fileDropArea")))
        throw StepError{QStringLiteral("not on the files step")};
    click(QStringLiteral("archiveNext"));
    return draft->step() == ArchiveDraft::DetailsStep ? QStringLiteral("ok") : QStringLiteral("blocked");
}

// The row of the scientific metadata whose names, from the top level down, are "path"; a group on the way that is
// closed is opened. Null when there is no such entry.
QQuickItem *ScenarioPlayer::metadataRow(const QStringList &path) const
{
    for (int attempt = 0; attempt < 8; ++attempt) {
        m_ui.settle();
        QStringList at; // the names above the row being looked at, and its own
        const QList<QQuickItem *> rows = m_ui.findAll(QStringLiteral("metadataRow"));
        QQuickItem *closed = nullptr;
        for (QQuickItem *row : rows) {
            const int depth = row->property("depth").toInt();
            at = at.mid(0, depth);
            at << row->property("key").toString();
            if (at == path)
                return row;
            if (at == path.mid(0, at.size()) && row->property("container").toBool() && !row->property("expanded").toBool()) {
                closed = row;
                break;
            }
        }
        if (!closed)
            return nullptr;
        // its first button opens and closes the group
        const QList<QQuickItem *> parts = m_ui.everything(closed);
        for (QQuickItem *part : parts) {
            if (part->inherits("QQuickAbstractButton")) {
                m_ui.click(reach(part));
                break;
            }
        }
    }
    return nullptr;
}

// "sample/name" = "S1": the value of an entry that is there is typed over; a missing last part is added to its group
void ScenarioPlayer::setMetadataEntry(const QString &path, const QString &value)
{
    QStringList names = path.split(QLatin1Char('/'));
    QQuickItem *row = metadataRow(names);
    if (!row) {
        const QString last = names.takeLast();
        QQuickItem *group = names.isEmpty() ? nullptr : metadataRow(names);
        if (names.isEmpty()) {
            click(QStringLiteral("metadataAddField"));
        } else if (group) {
            const QList<QQuickItem *> add = m_ui.findAll(QStringLiteral("metadataAddChild"), group);
            if (add.isEmpty())
                throw StepError{QStringLiteral("%1 is not a group").arg(names.join(QLatin1Char('/')))};
            m_ui.click(reach(add.constFirst()));
        } else {
            throw StepError{QStringLiteral("no metadata entry %1").arg(names.join(QLatin1Char('/')))};
        }
        // the new entry has no name yet
        row = metadataRow(names + QStringList{QString()});
        if (!row)
            throw StepError{QStringLiteral("no entry was added to %1").arg(names.join(QLatin1Char('/')))};
        m_ui.typeInto(reach(m_ui.findAll(QStringLiteral("metadataKey"), row).value(0)), last);
        names << last;
        row = metadataRow(names);
        if (!row)
            throw StepError{QStringLiteral("the new entry %1 is not listed").arg(path)};
    }
    const QList<QQuickItem *> field = m_ui.findAll(QStringLiteral("metadataValue"), row);
    if (field.isEmpty())
        throw StepError{QStringLiteral("%1 has no value to type").arg(path)};
    m_ui.typeInto(reach(field.constFirst()), value);
    m_ui.settle();
}

QString ScenarioPlayer::setMetadata(const QJsonObject &step)
{
    ArchiveDraft *draft = m_backend->archive();
    if (draft->step() != ArchiveDraft::DetailsStep || !maybe(QStringLiteral("datasetName")))
        throw StepError{QStringLiteral("not on the details step")};
    if (step.contains(QLatin1String("type"))) {
        const QString type = step.value(QLatin1String("type")).toString();
        if (type != QLatin1String("raw") && type != QLatin1String("derived"))
            throw StepError{QStringLiteral("unknown data type ") + type};
        click(type == QLatin1String("raw") ? QStringLiteral("rawChoice") : QStringLiteral("derivedChoice"));
    }
    if (step.contains(QLatin1String("instrument")))
        chooseEntry(QStringLiteral("instrumentBox"), step.value(QLatin1String("instrument")).toString());
    if (step.contains(QLatin1String("datasetName"))) {
        typeInto(QStringLiteral("datasetName"), step.value(QLatin1String("datasetName")).toString());
        key(QStringLiteral("Return"));
    }
    if (step.contains(QLatin1String("description")))
        typeInto(QStringLiteral("datasetDescription"), step.value(QLatin1String("description")).toString());
    if (step.contains(QLatin1String("investigator")))
        typeInto(QStringLiteral("investigator"), step.value(QLatin1String("investigator")).toString());
    if (step.contains(QLatin1String("usedSoftware")))
        typeInto(QStringLiteral("usedSoftware"), step.value(QLatin1String("usedSoftware")).toString());
    const QJsonObject metadata = step.value(QLatin1String("scientificMetadata")).toObject();
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it)
        setMetadataEntry(it.key(), it.value().toString());

    if (step.contains(QLatin1String("inputDatasets"))) {
        const QStringList names = stringList(step.value(QLatin1String("inputDatasets")));
        click(QStringLiteral("chooseInputDatasets"));
        if (!waitUntil([this] { return openDialog(QStringLiteral("pickerDialog")) != nullptr; }, 3000))
            return QStringLiteral("blocked");
        waitFor([draft] { return draft->pickerState() != ArchiveDraft::PickerLoading; }, 30000, QStringLiteral("the datasets to choose from"));
        for (const QString &name : names) {
            typeInto(QStringLiteral("pickerSearch"), name);
            QQuickItem *entry = nullptr;
            waitFor([this, &entry, &name] {
                        const QList<QQuickItem *> rows = m_ui.findAll(QStringLiteral("pickerRow"));
                        for (QQuickItem *row : rows) {
                            if (row->property("name").toString() == name) {
                                entry = row;
                                return true;
                            }
                        }
                        return false;
                    }, 5000, QStringLiteral("%1 to be listed").arg(name));
            if (!entry->property("checked").toBool())
                m_ui.click(reach(entry));
            m_ui.settle();
        }
        if (draft->pickedCount() != names.size())
            throw StepError{QStringLiteral("%1 of %2 input datasets are ticked").arg(draft->pickedCount()).arg(names.size())};
        click(QStringLiteral("pickerAdd"));
        waitFor([this] { return openDialog(QStringLiteral("pickerDialog")) == nullptr; }, 3000, QStringLiteral("the list of datasets to close"));
    }
    return QStringLiteral("ok");
}

QString ScenarioPlayer::submit(const QJsonObject &step)
{
    ArchiveDraft *draft = m_backend->archive();
    if (draft->step() == ArchiveDraft::DetailsStep)
        click(QStringLiteral("archiveNext"));
    if (draft->step() != ArchiveDraft::ReviewStep || !maybe(QStringLiteral("archiveSubmit")))
        return QStringLiteral("blocked");
    if (step.contains(QLatin1String("ingest")))
        setSwitch(QStringLiteral("ingestOption"), step.value(QLatin1String("ingest")).toBool());
    // the second option only counts, and can only be changed, together with the first
    if (step.contains(QLatin1String("archive")) && draft->ingest())
        setSwitch(QStringLiteral("archiveOption"), step.value(QLatin1String("archive")).toBool());
    const IngestRun *before = draft->run();
    click(QStringLiteral("archiveSubmit"));
    const IngestRun *run = draft->run();
    if (!run || run == before)
        return QStringLiteral("blocked");
    waitFor([run] { return !run->active(); }, 60000, QStringLiteral("the ingestion to finish"));
    m_ui.settle();
    return run->state() == IngestRun::Succeeded ? QStringLiteral("done") : QStringLiteral("failed");
}

// After a run that went through, "New dataset" starts with an empty form.
QString ScenarioPlayer::restart(const QJsonObject &)
{
    if (!maybe(QStringLiteral("runNew")))
        return QStringLiteral("blocked");
    click(QStringLiteral("runNew"));
    return m_backend->archive()->run() == nullptr ? QStringLiteral("ok") : QStringLiteral("blocked");
}

// ---------------------------------------------------------------------------------------------------------------
// retrieve

QString ScenarioPlayer::openRetrieve(const QJsonObject &)
{
    if (maybe(QStringLiteral("retrieveTile")))
        click(QStringLiteral("retrieveTile"));
    else
        click(QStringLiteral("navRetrieve"));
    pause(50);
    if (maybe(QStringLiteral("pgroupNeeded")))
        return QStringLiteral("blocked");
    const RetrieveController *retrieve = m_backend->retrieve();
    waitFor([retrieve] { return retrieve->loadState() == RetrieveController::Loaded || retrieve->loadState() == RetrieveController::Failed; },
            60000, QStringLiteral("the list of datasets"));
    m_ui.settle();
    return QStringLiteral("ok");
}

// Ticks the named datasets and nothing else, and opens the first of them, which shows its files and metadata next
// to the list. Each is looked up with the search box: the list only holds the rows in view.
QString ScenarioPlayer::selectDatasets(const QJsonObject &step)
{
    RetrieveController *retrieve = m_backend->retrieve();
    if (!maybe(QStringLiteral("datasetSearch")))
        throw StepError{QStringLiteral("the list of datasets is not shown")};
    if (retrieve->selectedCount() > 0)
        click(QStringLiteral("clearSelection"));
    const QStringList names = stringList(step.value(QLatin1String("datasets")));
    int ticked = 0;
    QString firstPid;
    for (const QString &name : names) {
        typeInto(QStringLiteral("datasetSearch"), name);
        QQuickItem *row = nullptr;
        const bool listed = waitUntil([this, retrieve, &row, &name] {
            if (retrieve->datasets()->search() != name)
                return false;
            const QList<QQuickItem *> rows = m_ui.findAll(QStringLiteral("datasetRow"));
            for (QQuickItem *candidate : rows) {
                if (candidate->property("name").toString() == name) {
                    row = candidate;
                    return true;
                }
            }
            return false;
        }, 5000);
        if (!listed)
            continue;
        const QList<QQuickItem *> box = m_ui.findAll(QStringLiteral("datasetRowCheck"), row);
        if (box.isEmpty())
            throw StepError{QStringLiteral("the row of %1 has no box to tick").arg(name)};
        m_ui.click(reach(box.constFirst()));
        m_ui.settle();
        ++ticked;
        if (firstPid.isEmpty()) {
            firstPid = row->property("pid").toString();
            // next to the box, on the row itself
            m_ui.click(reach(row));
            m_ui.settle();
        }
    }
    typeInto(QStringLiteral("datasetSearch"), QString());
    waitUntil([retrieve] { return retrieve->datasets()->search().isEmpty(); }, 3000);
    if (ticked != names.size())
        throw StepError{QStringLiteral("%1 of %2 datasets are listed").arg(ticked).arg(names.size())};
    if (!firstPid.isEmpty()) {
        const DatasetDetails *details = retrieve->details();
        waitUntil([details, &firstPid] {
            return details->pid() == firstPid && details->filesState() != DatasetDetails::Loading && details->filesState() != DatasetDetails::Idle
                   && details->metadataState() != DatasetDetails::Loading && details->metadataState() != DatasetDetails::Idle;
        }, 30000);
    }
    return QStringLiteral("ok");
}

QString ScenarioPlayer::requestRetrieve(const QJsonObject &)
{
    const RetrieveController *retrieve = m_backend->retrieve();
    // the buttons for a selection are only there while something is ticked
    if (!maybe(QStringLiteral("requestButton")))
        return unavailable;
    click(QStringLiteral("requestButton"));
    waitFor([retrieve] { return !retrieve->requesting(); }, 30000, QStringLiteral("the answer to the request"));
    m_ui.settle();
    return QStringLiteral("ok");
}

QString ScenarioPlayer::copyRetrieved(const QJsonObject &step)
{
    const QString to = step.value(QLatin1String("to")).toString();
    QDir().mkpath(to);
    RetrieveController *retrieve = m_backend->retrieve();
    const int before = int(m_backend->copies()->tasks().size());
    click(QStringLiteral("copyButton"));
    // the folder dialog belongs to the page, not to an item: it is found by its name
    QObject *folderDialog = nullptr;
    const QList<QObject *> named = m_ui.window()->findChildren<QObject *>(QStringLiteral("copyFolderDialog"));
    if (!named.isEmpty())
        folderDialog = named.constFirst();
    waitUntil([this, retrieve, folderDialog] {
        return !retrieve->checkingTicket()
               && (openDialog(QStringLiteral("ticketDialog")) || (folderDialog && folderDialog->property("visible").toBool())
                   || !retrieve->noticeText().isEmpty());
    }, 15000);
    if (openDialog(QStringLiteral("ticketDialog"))) {
        click(QStringLiteral("ticketCancel"));
        return QStringLiteral("blocked");
    }
    if (!folderDialog || !folderDialog->property("visible").toBool())
        return QStringLiteral("blocked");
    folderDialog->setProperty("selectedFolder", QUrl::fromLocalFile(to));
    QMetaObject::invokeMethod(folderDialog, "accept");
    waitUntil([this, before] { return m_backend->copies()->tasks().size() > before; }, 5000);
    m_ui.settle();
    return QStringLiteral("ok");
}

QString ScenarioPlayer::openJobs(const QJsonObject &)
{
    click(QStringLiteral("navActivity"));
    pause(50);
    return maybe(QStringLiteral("activityList")) || maybe(QStringLiteral("activityEmpty")) ? QStringLiteral("ok") : QStringLiteral("blocked");
}

// The list follows the copies by itself. "untilEmpty": until no copy is running any more.
QString ScenarioPlayer::refreshJobs(const QJsonObject &step)
{
    if (step.value(QLatin1String("untilEmpty")).toBool()) {
        const CopyQueue *copies = m_backend->copies();
        waitFor([copies] { return copies->activeCount() == 0; }, 20000, QStringLiteral("the copies to finish"));
    }
    m_ui.settle();
    return QStringLiteral("ok");
}

QString ScenarioPlayer::quit(const QJsonObject &)
{
    QQuickWindow *window = m_ui.window();
    window->close();
    pause(100);
    if (!m_gone && openDialog(QStringLiteral("quitDialog"))) {
        noteDialogs();
        click(m_answer == QLatin1String("no") ? QStringLiteral("quitCancel") : QStringLiteral("quitAnyway"));
        waitUntil([this] { return m_gone || openDialog(QStringLiteral("quitDialog")) == nullptr; }, 15000);
        pause(100);
    }
    return m_gone || !window->isVisible() ? QStringLiteral("closed") : QStringLiteral("stayed");
}

// ---------------------------------------------------------------------------------------------------------------
// snapshots: what the application shows, in the terms of the scenarios. Fields are read from the page where it shows
// them, lists from the application's own lists (a list on the screen only holds the rows in view).

static QJsonValue metadataTree(const MetadataModel *model)
{
    // rows are the open entries in order, each with its depth; a group that is closed hides its entries, so every
    // group is read from the model's own JSON instead
    const std::function<QJsonValue(const QJsonValue &)> shownAs = [&shownAs](const QJsonValue &value) -> QJsonValue {
        if (value.isObject()) {
            QJsonObject out;
            const QJsonObject object = value.toObject();
            for (auto it = object.constBegin(); it != object.constEnd(); ++it)
                out.insert(it.key(), shownAs(it.value()));
            return out;
        }
        if (value.isArray()) {
            QJsonArray out;
            const QJsonArray array = value.toArray();
            for (const QJsonValue &entry : array)
                out.append(shownAs(entry));
            return out;
        }
        // a value is shown as text: "6.2", "true"
        return value.isString() ? value : QJsonValue(value.toVariant().toString());
    };
    return shownAs(model->toJson());
}

QJsonValue ScenarioPlayer::snapshotValue(const QString &name) const
{
    m_ui.settle();
    const auto shownText = [this](const QString &field, const QString &kept) {
        const QQuickItem *item = maybe(field);
        return item ? textOf(item) : kept;
    };
    if (name == QLatin1String("session")) {
        const Session *session = m_backend->session();
        const QQuickItem *box = maybe(QStringLiteral("serverBox"));
        return QJsonObject{{QStringLiteral("signedIn"), maybe(QStringLiteral("navOverview")) != nullptr},
                           {QStringLiteral("username"), shownText(QStringLiteral("userName"), session->username())},
                           {QStringLiteral("server"), box ? box->property("currentText").toString() : session->serverName()}};
    }
    if (name == QLatin1String("pgroup")) {
        const Workspace *workspace = m_backend->workspace();
        const QQuickItem *email = maybe(QStringLiteral("piEmail"));
        const bool overview = maybe(QStringLiteral("groupBox")) != nullptr;
        // what the pgroup field says, also while another page is in front: a pgroup that was refused is still
        // written there
        const QQuickItem *box = m_ui.window()->findChild<QQuickItem *>(QStringLiteral("groupBox"));
        return QJsonObject{
            {QStringLiteral("pgroup"), box ? box->property("editText").toString() : workspace->pgroup()},
            {QStringLiteral("proposalId"), workspace->groupReady() ? workspace->proposalId() : QString()},
            {QStringLiteral("title"), workspace->groupReady() ? workspace->proposalTitle() : QString()},
            {QStringLiteral("piFirstname"), shownText(QStringLiteral("piFirstName"), workspace->groupReady() ? workspace->piFirstName() : QString())},
            {QStringLiteral("piLastname"), shownText(QStringLiteral("piLastName"), workspace->groupReady() ? workspace->piLastName() : QString())},
            {QStringLiteral("piEmail"), shownText(QStringLiteral("piEmail"), workspace->groupReady() ? workspace->piEmail() : QString())},
            {QStringLiteral("piEditable"), overview ? (email && email->isEnabled()) : (workspace->groupReady() && workspace->piEditable())},
            {QStringLiteral("proposalShown"), overview ? maybe(QStringLiteral("proposalCard")) != nullptr
                                                       : (workspace->groupReady() && workspace->source() == Workspace::FromCatalog)}};
    }
    const ArchiveDraft *draft = m_backend->archive();
    if (name == QLatin1String("archive.files"))
        return QJsonArray::fromStringList(draft->files()->paths());
    if (name == QLatin1String("archive.metadata")) {
        QJsonArray inputs;
        const InputDatasetModel *chosen = draft->inputDatasets();
        for (int row = 0; row < chosen->rowCount(); ++row) {
            inputs.append(QJsonObject{{QStringLiteral("name"), chosen->data(chosen->index(row), InputDatasetModel::NameRole).toString()},
                                      {QStringLiteral("pid"), chosen->data(chosen->index(row), InputDatasetModel::PidRole).toString()}});
        }
        const auto boxText = [this](const QString &box, const QString &kept) {
            const QQuickItem *item = maybe(box);
            return item ? item->property("currentText").toString() : kept;
        };
        return QJsonObject{
            {QStringLiteral("type"), draft->raw() ? QStringLiteral("raw") : QStringLiteral("derived")},
            {QStringLiteral("datasetName"), shownText(QStringLiteral("datasetName"), draft->datasetName())},
            {QStringLiteral("owner"), shownText(QStringLiteral("ownerName"), draft->owner())},
            {QStringLiteral("ownerGroup"), boxText(QStringLiteral("ownerGroupBox"), draft->ownerGroup())},
            {QStringLiteral("ownerEmail"), shownText(QStringLiteral("ownerEmail"), draft->ownerEmail())},
            {QStringLiteral("proposalId"), shownText(QStringLiteral("proposalId"), draft->proposalId())},
            {QStringLiteral("investigator"), shownText(QStringLiteral("investigator"), draft->investigator())},
            {QStringLiteral("instrument"), boxText(QStringLiteral("instrumentBox"), draft->creationLocation())},
            {QStringLiteral("description"), shownText(QStringLiteral("datasetDescription"), draft->description())},
            {QStringLiteral("usedSoftware"), shownText(QStringLiteral("usedSoftware"), draft->usedSoftware())},
            {QStringLiteral("inputDatasets"), inputs},
            // derived data has no scientific metadata to fill in
            {QStringLiteral("scientificMetadata"), draft->raw() ? metadataTree(draft->metadata()) : QJsonValue()}};
    }
    const RetrieveController *retrieve = m_backend->retrieve();
    if (name == QLatin1String("retrieve.list")) {
        const auto *list = qobject_cast<const DatasetListModel *>(retrieve->datasets()->sourceModel());
        std::map<QString, QJsonObject> rows;
        for (int row = 0; list && row < list->rowCount(); ++row) {
            const scicat::DatasetSummary &dataset = list->dataset(row);
            rows[dataset.pid] = QJsonObject{{QStringLiteral("name"), dataset.name},
                                            {QStringLiteral("retrievable"), dataset.retrievable},
                                            {QStringLiteral("retrieved"), list->state(row) == DatasetListModel::Ready},
                                            {QStringLiteral("pid"), dataset.pid}};
        }
        QJsonArray out;
        for (const auto &[pid, row] : rows)
            out.append(row);
        return out;
    }
    if (name == QLatin1String("retrieve.details")) {
        const DatasetDetails *details = retrieve->details();
        std::map<QString, QJsonObject> rows;
        for (const scicat::DataFile &file : details->files()->files()) {
            rows[file.path] = QJsonObject{{QStringLiteral("path"), file.path},
                                          {QStringLiteral("size"), double(file.sizeBytes)},
                                          {QStringLiteral("time"), file.time.toUTC().toString(Qt::ISODateWithMs)}};
        }
        QJsonArray files;
        for (const auto &[path, row] : rows)
            files.append(row);
        return QJsonObject{{QStringLiteral("files"), files},
                           {QStringLiteral("scientificMetadata"),
                            QJsonObject{{QStringLiteral("scientificMetadata"), metadataTree(details->metadata())}}}};
    }
    if (name == QLatin1String("jobs")) {
        // the copies that are running or waiting their turn
        QJsonArray rows;
        for (const CopyTask *task : m_backend->copies()->tasks()) {
            if (task->active()) {
                rows.append(QJsonObject{{QStringLiteral("name"), task->name()},
                                        {QStringLiteral("path"), task->folder()},
                                        {QStringLiteral("datasetId"), task->pid()}});
            }
        }
        return rows;
    }
    throw StepError{QStringLiteral("unknown snapshot ") + name};
}

QString ScenarioPlayer::snapshot(const QJsonObject &step)
{
    const QString name = step.value(QLatin1String("name")).toString();
    m_snapshots.insert(step.value(QLatin1String("as")).toString(name), snapshotValue(name));
    return QStringLiteral("ok");
}
