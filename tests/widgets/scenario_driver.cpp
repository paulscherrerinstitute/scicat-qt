// Scenario driver for the Widgets GUI (see tests/parity/README.md, "Driver protocol").
//
// Linked into the test copy of the application next to its unchanged main(). When the QApplication starts, it
// reads the scenario named by SCICAT_TEST_SCENARIO, plays its steps on the real Login and Dialog windows by their
// object names (as a user would: typing into fields, clicking buttons, answering dialogs), and writes what it
// observed to SCICAT_TEST_OUT. It asserts nothing; tests/parity/run_scenarios.py compares.
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <functional>
#include <map>
#include <utility>

namespace {

// pages of Dialog::stackedWidget
constexpr int PagePgroup = 0, PageFiles = 1, PageMetadata = 2, PageSubmit = 3, PageRetrieve = 4, PageJobs = 5;

struct StepError {
    QString message;
};

class Driver : public QObject
{
public:
    void start();

private:
    // bookkeeping
    QJsonObject scenario;
    QJsonArray stepResults, dialogs, banners;
    QJsonObject snapshots;
    QString lastBanner;
    QPointer<QWidget> handledModal;
    std::function<void(QWidget*)> dialogHandler;
    QString dialogHandlerFor;
    QString answer = "yes";
    bool inHandler = false;

    void save() const;
    void run();
    void tick();
    void pollBanners();
    QString runStep(const QJsonObject& step);

    // helpers
    static QWidget* window(const char* name);
    QWidget* mainWindow() const;
    template <class T> static T* child(QWidget* root, const char* name);
    static void waitFor(const std::function<bool()>& done, int ms, const QString& what);
    static void settle(int ms = 20);
    void expectDialog(const QString& kind, std::function<void(QWidget*)> handler);
    void click(QAbstractButton* button);
    static void type(QLineEdit* edit, const QString& text);
    int page() const;

    // steps
    QString signIn(const QJsonObject& s);
    QString changeSettings(const QJsonObject& s);
    QString choosePgroup(const QJsonObject& s);
    QString enterPI(const QJsonObject& s);
    QString openArchive(const QJsonObject& s);
    QString addFiles(const QJsonObject& s);
    QString toMetadata(const QJsonObject& s);
    QString setMetadata(const QJsonObject& s);
    QString submit(const QJsonObject& s);
    QString restart(const QJsonObject& s);
    QString openRetrieve(const QJsonObject& s);
    QString selectDatasets(const QJsonObject& s);
    QString requestRetrieve(const QJsonObject& s);
    QString copyRetrieved(const QJsonObject& s);
    QString openJobs(const QJsonObject& s);
    QString refreshJobs(const QJsonObject& s);
    QString quit(const QJsonObject& s);
    QString snapshot(const QJsonObject& s);

    QJsonValue snapshotValue(const QString& name);
};

// ---------------------------------------------------------------------------------------------------------------

QWidget* Driver::window(const char* name)
{
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (w->objectName() == QLatin1String(name) && w->isVisible()) {
            return w;
        }
    }
    return nullptr;
}

QWidget* Driver::mainWindow() const
{
    QWidget* w = window("Dialog");
    if (!w) {
        throw StepError{"the main window is not open"};
    }
    return w;
}

template <class T> T* Driver::child(QWidget* root, const char* name)
{
    T* w = root ? root->findChild<T*>(QLatin1String(name)) : nullptr;
    if (!w) {
        throw StepError{QString("no %1 named %2").arg(T::staticMetaObject.className(), name)};
    }
    return w;
}

void Driver::settle(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::msleep(2);
    }
}

void Driver::waitFor(const std::function<bool()>& done, int ms, const QString& what)
{
    QElapsedTimer t;
    t.start();
    while (!done()) {
        if (t.elapsed() > ms) {
            throw StepError{"timed out waiting for " + what};
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::msleep(2);
    }
}

// the last click of a dialog handler runs after the handler, so a message box it opens is answered as usual
static void later(QAbstractButton* button)
{
    QTimer::singleShot(0, button, &QAbstractButton::click);
}

void Driver::type(QLineEdit* edit, const QString& text)
{
    if (!edit->isEnabled()) {
        throw StepError{edit->objectName() + " cannot be edited"};
    }
    edit->setText(text);
    emit edit->editingFinished();
}

void Driver::click(QAbstractButton* button)
{
    if (!button->isEnabled() || button->isHidden()) {
        throw StepError{button->objectName() + " cannot be clicked"};
    }
    button->click();
    settle();
}

int Driver::page() const
{
    return child<QStackedWidget>(mainWindow(), "stackedWidget")->currentIndex();
}

void Driver::expectDialog(const QString& kind, std::function<void(QWidget*)> handler)
{
    dialogHandlerFor = kind;
    dialogHandler = std::move(handler);
}

// called every few milliseconds, also inside the nested event loops of modal dialogs and network requests
void Driver::tick()
{
    if (inHandler) {
        return;
    }
    QWidget* modal = QApplication::activeModalWidget();
    if (!modal || modal == handledModal) {
        return;
    }
    handledModal = modal;
    if (auto* box = qobject_cast<QMessageBox*>(modal)) {
        // only the text: macOS does not keep the title of a message box
        dialogs.append(QJsonObject{{"text", box->text()}});
        QAbstractButton* button = nullptr;
        if (box->button(QMessageBox::Yes) && box->button(QMessageBox::No)) {
            button = box->button(answer == "no" ? QMessageBox::No : QMessageBox::Yes);
        } else if (!box->buttons().isEmpty()) {
            button = box->defaultButton() ? box->defaultButton() : box->buttons().first();
        }
        if (button) {
            button->click();
        } else {
            box->reject();
        }
        return;
    }
    const QString kind = qobject_cast<QFileDialog*>(modal) ? QString("file") : modal->objectName();
    if (dialogHandler && kind == dialogHandlerFor) {
        auto handler = std::exchange(dialogHandler, {});
        inHandler = true;
        try {
            handler(modal);
        } catch (const StepError& e) {
            dialogs.append(QJsonObject{{"error", e.message}, {"dialog", kind}});
            if (auto* d = qobject_cast<QDialog*>(modal)) {
                d->reject();
            } else {
                modal->close();
            }
        }
        inHandler = false;
        return;
    }
    // a dialog the scenario did not ask for: note it and close it, so the run cannot hang
    dialogs.append(QJsonObject{{"unexpected", kind}, {"title", modal->windowTitle()}});
    if (auto* d = qobject_cast<QDialog*>(modal)) {
        d->reject();
    } else {
        modal->close();
    }
}

// the message line of the window at the end of a step (it stays for 5 s); a message that is replaced within the
// step, such as "Retrieving dataset info...", is not recorded
void Driver::pollBanners()
{
    QLabel* label = nullptr;
    if (QWidget* d = window("Dialog")) {
        label = d->findChild<QLabel*>("warningLabel");
    } else if (QWidget* l = window("Login")) {
        label = l->findChild<QLabel*>("labelErrorMessage");
    }
    if (!label || label->isHidden() || label->text().isEmpty()) {
        lastBanner.clear();
        return;
    }
    if (label->text() != lastBanner) {
        lastBanner = label->text();
        banners.append(lastBanner);
    }
}

void Driver::save() const
{
    QFile out(qEnvironmentVariable("SCICAT_TEST_OUT"));
    if (out.open(QIODevice::WriteOnly)) {
        QJsonObject messages{{"dialogs", dialogs}, {"banners", banners}};
        out.write(QJsonDocument(QJsonObject{{"driver", "widgets"},
                                            {"steps", stepResults},
                                            {"messages", messages},
                                            {"snapshots", snapshots}})
                      .toJson());
    }
}

void Driver::start()
{
    QFile file(qEnvironmentVariable("SCICAT_TEST_SCENARIO"));
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("scenario driver: cannot read SCICAT_TEST_SCENARIO");
    }
    scenario = QJsonDocument::fromJson(file.readAll()).object();
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { tick(); });
    timer->start(5);
    QTimer::singleShot(0, this, [this] { run(); });
}

void Driver::run()
{
    settle(50);
    for (const auto& value : scenario.value("steps").toArray()) {
        const QJsonObject step = value.toObject();
        answer = step.value("answer").toString("yes");
        QString result;
        try {
            result = runStep(step);
        } catch (const StepError& e) {
            result = "error: " + e.message;
        }
        if (dialogHandler) {
            // the step expected a dialog that never opened
            dialogHandler = {};
        }
        settle();
        pollBanners();
        stepResults.append(QJsonObject{{"do", step.value("do")}, {"result", result}});
        save();
    }
    save();
    QCoreApplication::exit(0);
}

QString Driver::runStep(const QJsonObject& step)
{
    static const std::map<QString, QString (Driver::*)(const QJsonObject&)> steps{
        {"signIn", &Driver::signIn},
        {"changeSettings", &Driver::changeSettings},
        {"choosePgroup", &Driver::choosePgroup},
        {"enterPI", &Driver::enterPI},
        {"archive.open", &Driver::openArchive},
        {"archive.addFiles", &Driver::addFiles},
        {"archive.toMetadata", &Driver::toMetadata},
        {"archive.setMetadata", &Driver::setMetadata},
        {"archive.submit", &Driver::submit},
        {"archive.restart", &Driver::restart},
        {"retrieve.open", &Driver::openRetrieve},
        {"retrieve.select", &Driver::selectDatasets},
        {"retrieve.request", &Driver::requestRetrieve},
        {"retrieve.copy", &Driver::copyRetrieved},
        {"jobs.open", &Driver::openJobs},
        {"jobs.refresh", &Driver::refreshJobs},
        {"quit", &Driver::quit},
        {"snapshot", &Driver::snapshot},
    };
    const QString name = step.value("do").toString();
    if (name == "waitForFiles") {
        const int timeout = step.value("timeout").toInt(20000);
        for (const auto& path : step.value("paths").toArray()) {
            waitFor([p = path.toString()] { return QFileInfo::exists(p); }, timeout, path.toString());
        }
        return "ok";
    }
    if (name == "removeFile") {
        QFile::remove(step.value("path").toString());
        return "ok";
    }
    if (name == "wait") {
        settle(step.value("ms").toInt());
        return "ok";
    }
    const auto it = steps.find(name);
    if (it == steps.end()) {
        throw StepError{"unknown step " + name};
    }
    return (this->*(it->second))(step);
}

// ---------------------------------------------------------------------------------------------------------------
// sign in and settings

QString Driver::signIn(const QJsonObject& s)
{
    QWidget* login = window("Login");
    if (!login) {
        throw StepError{"the sign-in window is not open"};
    }
    type(child<QLineEdit>(login, "lineEditPassword"), s.value("token").toString());
    click(child<QPushButton>(login, "pushButtonLogin"));
    return window("Dialog") ? "signedIn" : "rejected";
}

QString Driver::changeSettings(const QJsonObject& s)
{
    QWidget* owner = window("Dialog") ? window("Dialog") : window("Login");
    if (!owner) {
        throw StepError{"no window to open the settings from"};
    }
    const QJsonObject values = s.value("values").toObject();
    const bool reset = s.value("reset").toBool();
    const bool cancel = s.value("cancel").toBool();
    bool opened = false;
    expectDialog("Settings", [&](QWidget* dlg) {
        opened = true;
        static const std::map<QString, const char*> checkBoxes{
            {"deleteLog", "checkBoxDeleteLog"},           {"loadProposalInfo", "checkBoxLoadProposalInfo"},
            {"saveProposalInfo", "checkBoxSaveProposalInfo"}, {"saveLog", "checkBoxSaveLog"},
            {"enableArchive", "checkBoxEnableArchive"},   {"enableIngest", "checkBoxEnableIngest"},
            {"usePgroupAsDefaultPath", "checkBoxUsePgroupAsPath"}, {"investigatorIsPI", "checkBoxInvestigatorIsPI"},
            {"guessDatasetname", "checkBoxGuessDatasetname"}, {"printIngestion", "checkBoxPrintCalls"},
            {"disableSystemCalls", "checkBoxSystemCalls"}};
        static const std::map<QString, const char*> lineEdits{
            {"filenameMetadata", "lineEditMetadataFile"}, {"filenameFilelisting", "lineEditFilelistingFile"},
            {"logPath", "lineEditLogPath"}};
        if (reset) {
            child<QPushButton>(dlg, "pushButtonReset")->click();
        }
        for (auto it = values.begin(); it != values.end(); ++it) {
            if (it.key() == "serverID") {
                child<QCheckBox>(dlg, "checkBoxEnableAdvanced")->setChecked(true);
                child<QComboBox>(dlg, "comboBoxServer")->setCurrentIndex(it.value().toInt());
            } else if (checkBoxes.count(it.key())) {
                if (it.key() == "printIngestion" || it.key() == "disableSystemCalls") {
                    child<QCheckBox>(dlg, "checkBoxEnableAdvanced")->setChecked(true);
                }
                child<QCheckBox>(dlg, checkBoxes.at(it.key()))->setChecked(it.value().toBool());
            } else if (lineEdits.count(it.key())) {
                type(child<QLineEdit>(dlg, lineEdits.at(it.key())), it.value().toString());
            } else {
                throw StepError{"unknown setting " + it.key()};
            }
        }
        auto* box = child<QDialogButtonBox>(dlg, "buttonBox");
        later(box->button(cancel ? QDialogButtonBox::Cancel : QDialogButtonBox::Ok));
    });
    click(child<QPushButton>(owner, "pushButtonSettings"));
    if (!opened) {
        throw StepError{"the settings did not open"};
    }
    return "ok";
}

// ---------------------------------------------------------------------------------------------------------------
// pgroup

QString Driver::choosePgroup(const QJsonObject& s)
{
    type(child<QLineEdit>(mainWindow(), "lineEditPgroup"), s.value("pgroup").toString());
    settle();
    return "ok";
}

QString Driver::enterPI(const QJsonObject& s)
{
    QWidget* w = mainWindow();
    if (s.contains("firstname")) {
        type(child<QLineEdit>(w, "lineEditPIFirstname"), s.value("firstname").toString());
    }
    if (s.contains("lastname")) {
        type(child<QLineEdit>(w, "lineEditPILastname"), s.value("lastname").toString());
    }
    if (s.contains("email")) {
        type(child<QLineEdit>(w, "lineEditPIMail"), s.value("email").toString());
    }
    return "ok";
}

// ---------------------------------------------------------------------------------------------------------------
// archive

QString Driver::openArchive(const QJsonObject&)
{
    click(child<QPushButton>(mainWindow(), "pushButtonGoToArchive"));
    return page() == PageFiles ? "ok" : "blocked";
}

QString Driver::addFiles(const QJsonObject& s)
{
    const QString dir = s.value("dir").toString();
    QStringList names;
    for (const auto& n : s.value("names").toArray()) {
        names << n.toString();
    }
    bool opened = false;
    expectDialog("file", [&](QWidget* w) {
        opened = true;
        auto* fd = qobject_cast<QFileDialog*>(w);
        fd->setDirectory(dir);
        auto* view = child<QListView>(fd, "listView");
        QModelIndexList found;
        waitFor(
            [&] {
                found.clear();
                const QModelIndex root = view->rootIndex();
                const int rows = view->model()->rowCount(root);
                for (int r = 0; r < rows; ++r) {
                    const QModelIndex index = view->model()->index(r, 0, root);
                    if (names.contains(index.data().toString())) {
                        found << index;
                    }
                }
                return found.size() == names.size();
            },
            5000, "the files to show in the file dialog");
        view->selectionModel()->clearSelection();
        for (const auto& index : found) {
            view->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        // the same button FileDialogFilelisting takes over
        for (auto* b : fd->findChildren<QPushButton*>()) {
            const QString text = b->text().toLower();
            if (text.contains("open") || text.contains("choose")) {
                later(b);
                return;
            }
        }
        throw StepError{"no Open button in the file dialog"};
    });
    click(child<QPushButton>(mainWindow(), "addFilesDirectoriesButton"));
    if (!opened) {
        throw StepError{"the file dialog did not open"};
    }
    return "ok";
}

QString Driver::toMetadata(const QJsonObject&)
{
    if (page() != PageFiles) {
        throw StepError{"not on the files page"};
    }
    click(child<QPushButton>(mainWindow(), "nextSlideButton"));
    return page() == PageMetadata ? "ok" : "blocked";
}

static QTreeWidgetItem* findTreeItem(QTreeWidget* tree, const QStringList& path)
{
    QTreeWidgetItem* item = tree->invisibleRootItem();
    for (const auto& part : path) {
        QTreeWidgetItem* next = nullptr;
        for (int i = 0; i < item->childCount() && !next; ++i) {
            if (item->child(i)->text(0) == part) {
                next = item->child(i);
            }
        }
        if (!next) {
            return nullptr;
        }
        item = next;
    }
    return item;
}

QString Driver::setMetadata(const QJsonObject& s)
{
    QWidget* w = mainWindow();
    if (page() != PageMetadata) {
        throw StepError{"not on the metadata page"};
    }
    if (s.contains("type")) {
        auto* combo = child<QComboBox>(w, "comboBoxDatatype");
        const int index = combo->findText(s.value("type").toString());
        if (index < 0) {
            throw StepError{"unknown data type " + s.value("type").toString()};
        }
        combo->setCurrentIndex(index);
    }
    if (s.contains("instrument")) {
        auto* combo = child<QComboBox>(w, "comboBoxCreationLocation");
        const int index = combo->findText(s.value("instrument").toString());
        if (index < 0) {
            throw StepError{"unknown instrument " + s.value("instrument").toString()};
        }
        combo->setCurrentIndex(index);
    }
    if (s.contains("datasetName")) {
        type(child<QLineEdit>(w, "lineEditDatasetName"), s.value("datasetName").toString());
    }
    if (s.contains("description")) {
        child<QTextEdit>(w, "textEditDescription")->setPlainText(s.value("description").toString());
    }
    if (s.contains("investigator")) {
        type(child<QLineEdit>(w, "lineEditInvestigator"), s.value("investigator").toString());
    }
    if (s.contains("usedSoftware")) {
        child<QTextEdit>(w, "textEditUsedSoftware")->setPlainText(s.value("usedSoftware").toString());
    }
    // "sample/name": "S1" sets an existing entry; a missing last part is added with the + button
    const QJsonObject sm = s.value("scientificMetadata").toObject();
    auto* tree = child<QTreeWidget>(w, "treeWidget");
    for (auto it = sm.begin(); it != sm.end(); ++it) {
        QStringList path = QStringList{"scientificMetadata"} + it.key().split('/');
        QTreeWidgetItem* item = findTreeItem(tree, path);
        if (!item) {
            const QString key = path.takeLast();
            QTreeWidgetItem* parent = findTreeItem(tree, path);
            if (!parent) {
                throw StepError{"no metadata entry " + path.join('/')};
            }
            QSet<QTreeWidgetItem*> before;
            for (int i = 0; i < parent->childCount(); ++i) {
                before.insert(parent->child(i));
            }
            tree->setCurrentItem(parent);
            parent->setSelected(true);
            click(child<QToolButton>(w, "toolButtonPlus"));
            for (int i = 0; i < parent->childCount() && !item; ++i) {
                if (!before.contains(parent->child(i))) {
                    item = parent->child(i);
                }
            }
            if (!item) {
                throw StepError{"the + button added no entry under " + path.join('/')};
            }
            tree->closePersistentEditor(item, 0);
            item->setText(0, key);
        }
        item->setText(1, it.value().toString());
    }
    if (s.contains("inputDatasets")) {
        QStringList names;
        for (const auto& n : s.value("inputDatasets").toArray()) {
            names << n.toString();
        }
        bool opened = false;
        expectDialog("DatasetBrowser", [&](QWidget* dlg) {
            opened = true;
            auto* view = child<QTableView>(dlg, "tableViewDatasetBrowser");
            int selected = 0;
            for (int r = 0; r < view->model()->rowCount(); ++r) {
                if (names.contains(view->model()->index(r, 0).data().toString())) {
                    view->selectionModel()->select(view->model()->index(r, 0),
                                                   QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    ++selected;
                }
            }
            if (selected != names.size()) {
                throw StepError{"not all input datasets are listed"};
            }
            later(child<QPushButton>(dlg, "pushButton"));
        });
        click(child<QPushButton>(w, "pushButtonAddDatasets"));
        if (!opened) {
            return "blocked";
        }
    }
    return "ok";
}

QString Driver::submit(const QJsonObject& s)
{
    QWidget* w = mainWindow();
    if (page() == PageMetadata) {
        click(child<QPushButton>(w, "nextSlideButton"));
    }
    if (page() != PageSubmit) {
        return "blocked";
    }
    if (s.contains("ingest")) {
        child<QCheckBox>(w, "checkBoxIngest")->setChecked(s.value("ingest").toBool());
    }
    if (s.contains("archive")) {
        child<QCheckBox>(w, "checkBoxArchive")->setChecked(s.value("archive").toBool());
    }
    auto* status = child<QLabel>(w, "pleaseWait");
    click(child<QPushButton>(w, "pushButtonRestart"));
    waitFor([&] { return status->text() == "Done." || status->text() == "Ingestion failed." || page() != PageSubmit; },
            30000, "the ingestion to finish");
    if (page() != PageSubmit) {
        return "blocked";
    }
    return status->text() == "Done." ? "done" : "failed";
}

QString Driver::restart(const QJsonObject&)
{
    click(child<QPushButton>(mainWindow(), "pushButtonRestart"));
    return page() == PagePgroup ? "ok" : "blocked";
}

// ---------------------------------------------------------------------------------------------------------------
// retrieve

QString Driver::openRetrieve(const QJsonObject&)
{
    click(child<QPushButton>(mainWindow(), "pushButtonGoToRetrieve"));
    return page() == PageRetrieve ? "ok" : "blocked";
}

QString Driver::selectDatasets(const QJsonObject& s)
{
    auto* table = child<QTableWidget>(mainWindow(), "tableWidgetRetriever");
    QStringList names;
    for (const auto& n : s.value("datasets").toArray()) {
        names << n.toString();
    }
    table->clearSelection();
    settle();
    int selected = 0;
    for (int r = 0; r < table->rowCount(); ++r) {
        if (names.contains(table->item(r, 0)->text())) {
            table->selectionModel()->select(table->model()->index(r, 0),
                                            QItemSelectionModel::Select | QItemSelectionModel::Rows);
            ++selected;
            settle();
        }
    }
    if (selected != names.size()) {
        throw StepError{QString("%1 of %2 datasets are listed").arg(selected).arg(names.size())};
    }
    return "ok";
}

QString Driver::requestRetrieve(const QJsonObject&)
{
    click(child<QPushButton>(mainWindow(), "pushButtonRetrieve"));
    return "ok";
}

QString Driver::copyRetrieved(const QJsonObject& s)
{
    const QString to = s.value("to").toString();
    QDir().mkpath(to);
    bool opened = false;
    expectDialog("file", [&](QWidget* w) {
        opened = true;
        auto* fd = qobject_cast<QFileDialog*>(w);
        fd->setDirectory(to);
        fd->selectFile(to);
        QTimer::singleShot(0, fd, [fd] { QMetaObject::invokeMethod(fd, "accept"); });
    });
    click(child<QPushButton>(mainWindow(), "pushButtonSave"));
    return opened ? "ok" : "blocked";
}

QString Driver::openJobs(const QJsonObject&)
{
    click(child<QPushButton>(mainWindow(), "pushButtonJobs"));
    return page() == PageJobs ? "ok" : "blocked";
}

// "untilEmpty": refresh until no copy is listed any more (up to 20 s), instead of a fixed wait
QString Driver::refreshJobs(const QJsonObject& s)
{
    auto* refresh = child<QPushButton>(mainWindow(), "pushButtonRefreshJobs");
    auto* table = child<QTableWidget>(mainWindow(), "tableWidgetJobs");
    click(refresh);
    if (s.value("untilEmpty").toBool()) {
        waitFor([&] { click(refresh); return table->rowCount() == 0; }, 20000, "the copies to finish");
    }
    return "ok";
}

QString Driver::quit(const QJsonObject&)
{
    mainWindow()->close();
    settle(100);
    return window("Dialog") ? "stayed" : "closed";
}

// ---------------------------------------------------------------------------------------------------------------
// snapshots: what the GUI shows, in terms that do not depend on the toolkit

static QJsonObject treeToJson(QTreeWidgetItem* item)
{
    QJsonObject out;
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* c = item->child(i);
        out.insert(c->text(0), c->childCount() > 0 ? QJsonValue(treeToJson(c)) : QJsonValue(c->text(1)));
    }
    return out;
}

static QString afterColon(const QString& text)
{
    const int at = text.indexOf(": ");
    return at < 0 ? text : text.mid(at + 2);
}

QJsonValue Driver::snapshotValue(const QString& name)
{
    if (name == "session") {
        QWidget* d = window("Dialog");
        QWidget* l = window("Login");
        return QJsonObject{
            {"signedIn", d != nullptr},
            {"username", d ? afterColon(child<QLabel>(d, "labelUsername")->text()) : QString()},
            {"server", afterColon(d ? child<QLabel>(d, "labelServer")->text()
                                    : l ? child<QLabel>(l, "labelServer")->text() : QString())}};
    }
    QWidget* w = mainWindow();
    if (name == "pgroup") {
        return QJsonObject{
            {"pgroup", child<QLineEdit>(w, "lineEditPgroup")->text()},
            {"proposalId", child<QLineEdit>(w, "lineEditProposalID_2")->text()},
            {"title", child<QLineEdit>(w, "lineEditTitle")->text()},
            {"piFirstname", child<QLineEdit>(w, "lineEditPIFirstname")->text()},
            {"piLastname", child<QLineEdit>(w, "lineEditPILastname")->text()},
            {"piEmail", child<QLineEdit>(w, "lineEditPIMail")->text()},
            {"piEditable", child<QLineEdit>(w, "lineEditPIMail")->isEnabled()},
            {"proposalShown", !child<QLabel>(w, "labelProposal")->isHidden()}};
    }
    if (name == "archive.files") {
        QJsonArray files;
        auto* list = child<QListWidget>(w, "filelistingListWidget");
        for (int i = 0; i < list->count(); ++i) {
            files.append(list->item(i)->text());
        }
        return files;
    }
    if (name == "archive.metadata") {
        QJsonArray inputs;
        auto* table = child<QTableWidget>(w, "tableWidget");
        for (int r = 0; r < table->rowCount(); ++r) {
            inputs.append(QJsonObject{{"name", table->item(r, 0) ? table->item(r, 0)->text() : QString()},
                                      {"pid", table->item(r, 1) ? table->item(r, 1)->text() : QString()}});
        }
        QTreeWidgetItem* sm = findTreeItem(child<QTreeWidget>(w, "treeWidget"), {"scientificMetadata"});
        return QJsonObject{
            {"type", child<QComboBox>(w, "comboBoxDatatype")->currentText()},
            {"datasetName", child<QLineEdit>(w, "lineEditDatasetName")->text()},
            {"owner", child<QLineEdit>(w, "lineEditOwner")->text()},
            {"ownerGroup", child<QLineEdit>(w, "lineEditOwnerGroup")->text()},
            {"ownerEmail", child<QLineEdit>(w, "lineEditOwnerEmail")->text()},
            {"proposalId", child<QLineEdit>(w, "lineEditProposalID")->text()},
            {"investigator", child<QLineEdit>(w, "lineEditInvestigator")->text()},
            {"instrument", child<QComboBox>(w, "comboBoxCreationLocation")->currentText()},
            {"description", child<QTextEdit>(w, "textEditDescription")->toPlainText()},
            {"usedSoftware", child<QTextEdit>(w, "textEditUsedSoftware")->toPlainText()},
            {"inputDatasets", inputs},
            {"scientificMetadata", sm ? QJsonValue(treeToJson(sm)) : QJsonValue()}};
    }
    if (name == "retrieve.list") {
        auto* table = child<QTableWidget>(w, "tableWidgetRetriever");
        std::map<QString, QJsonObject> rows;
        for (int r = 0; r < table->rowCount(); ++r) {
            rows[table->item(r, 3)->text()] = QJsonObject{{"name", table->item(r, 0)->text()},
                                                          {"retrievable", table->item(r, 1)->text() == "true"},
                                                          {"retrieved", table->item(r, 2)->text() == "true"},
                                                          {"pid", table->item(r, 3)->text()}};
        }
        QJsonArray out;
        for (const auto& [pid, row] : rows) {
            out.append(row);
        }
        return out;
    }
    if (name == "retrieve.details") {
        auto* table = child<QTableWidget>(w, "tableWidgetRetrieverFiles");
        std::map<QString, QJsonObject> rows;
        for (int r = 0; r < table->rowCount(); ++r) {
            rows[table->item(r, 0)->text()] = QJsonObject{{"path", table->item(r, 0)->text()},
                                                          {"size", table->item(r, 1)->text().toLongLong()},
                                                          {"time", table->item(r, 2)->text()}};
        }
        QJsonArray files;
        for (const auto& [path, row] : rows) {
            files.append(row);
        }
        return QJsonObject{
            {"files", files},
            {"scientificMetadata",
             treeToJson(child<QTreeWidget>(w, "treeWidgetScientificMetadataRetrieve")->invisibleRootItem())}};
    }
    if (name == "jobs") {
        auto* table = child<QTableWidget>(w, "tableWidgetJobs");
        QJsonArray rows;
        for (int r = 0; r < table->rowCount(); ++r) {
            rows.append(QJsonObject{{"name", table->item(r, 0)->text()},
                                    {"path", table->item(r, 1)->text()},
                                    {"datasetId", table->item(r, 3)->text()}});
        }
        return rows;
    }
    throw StepError{"unknown snapshot " + name};
}

QString Driver::snapshot(const QJsonObject& s)
{
    const QString name = s.value("name").toString();
    snapshots.insert(s.value("as").toString(name), snapshotValue(name));
    return "ok";
}

void startDriver()
{
    if (!qEnvironmentVariableIsSet("SCICAT_TEST_SCENARIO")) {
        return;
    }
    auto* driver = new Driver;
    driver->setParent(QCoreApplication::instance());
    driver->start();
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(startDriver)
