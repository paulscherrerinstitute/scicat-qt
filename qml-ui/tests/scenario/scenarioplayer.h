#pragma once

#include "uiprobe.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <functional>

class Backend;

// Plays one scenario of tests/parity (see its README, "Driver protocol") on the window of the running application
// and writes down what it saw. It asserts nothing; tests/parity/run_scenarios.py compares.
//
// The steps are done the way a person does them: typing into the fields, clicking the buttons and the entries of
// lists, dropping files on the page. Three things are done differently:
//   - files are dropped on the Archive page, as from a file manager, instead of being picked in the file dialog;
//   - the folder a copy goes to is handed to the folder dialog and the dialog accepted, instead of browsing to it
//     (a test cannot work a file dialog of the system);
//   - an entry of a long drop-down list is brought into view before it is clicked.
// When to go on (a request has been answered, scicat-cli has ended) is read from the application's own state, and so
// is what a snapshot says where the page shows it in a list that only holds the rows in view.
class ScenarioPlayer : public QObject
{
    Q_OBJECT
public:
    // settingsFile: where the preferences are written for the comparison, in the layout of the Widgets GUI's file
    ScenarioPlayer(QQuickWindow *window, Backend *backend, const QString &settingsFile, QObject *parent = nullptr);

    void start();

    // The scenario's settings file made into one this application reads; nothing is written when there is none.
    static void importSettings(const QString &given, const QString &own);

private:
    void run();
    void save() const;
    void exportSettings() const;
    QString runStep(const QJsonObject &step);

    // what is on the screen
    QQuickItem *maybe(const QString &name, int nth = 0) const;
    QQuickItem *need(const QString &name, int nth = 0) const;
    QQuickItem *reach(const QString &name, int nth = 0) const;
    QQuickItem *reach(QQuickItem *item) const;
    void click(const QString &name, int nth = 0) const;
    void typeInto(const QString &name, const QString &text) const;
    void key(const QString &name) const;
    void chooseEntry(const QString &comboName, const QString &text) const;
    void setSwitch(const QString &name, bool on) const;
    QObject *openDialog(const QString &name) const;
    QList<QObject *> openDialogs() const;
    QStringList messagesShown() const;
    void noteDialogs();
    void noteMessages();

    // waiting
    void pause(int ms);
    bool waitUntil(const std::function<bool()> &done, int ms);
    void waitFor(const std::function<bool()> &done, int ms, const QString &what);

    // steps
    QString signIn(const QJsonObject &step);
    QString changeSettings(const QJsonObject &step);
    QString choosePgroup(const QJsonObject &step);
    QString enterPI(const QJsonObject &step);
    QString openArchive(const QJsonObject &step);
    QString addFiles(const QJsonObject &step);
    QString toMetadata(const QJsonObject &step);
    QString setMetadata(const QJsonObject &step);
    QString submit(const QJsonObject &step);
    QString restart(const QJsonObject &step);
    QString openRetrieve(const QJsonObject &step);
    QString selectDatasets(const QJsonObject &step);
    QString requestRetrieve(const QJsonObject &step);
    QString copyRetrieved(const QJsonObject &step);
    QString openJobs(const QJsonObject &step);
    QString refreshJobs(const QJsonObject &step);
    QString quit(const QJsonObject &step);
    QString snapshot(const QJsonObject &step);
    QJsonValue snapshotValue(const QString &name) const;
    void setMetadataEntry(const QString &path, const QString &value);
    QQuickItem *metadataRow(const QStringList &path) const;

    UiProbe m_ui;
    Backend *m_backend;
    QString m_settingsFile;
    QJsonObject m_scenario;
    QJsonArray m_steps;
    QJsonArray m_dialogs;
    QJsonArray m_banners;
    QJsonObject m_snapshots;
    QSet<QObject *> m_dialogsSeen;
    QStringList m_messagesBefore;
    QString m_answer;
    bool m_gone = false; // the application is quitting or has quit
};
