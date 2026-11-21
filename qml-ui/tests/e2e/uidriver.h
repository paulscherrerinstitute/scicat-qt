#pragma once

#include "uiprobe.h"

#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <functional>

class Backend;

// Plays a JSON script against the running application: real mouse and key events on items found by objectName,
// checks on their properties, and screenshots.
//
// A script is an array of steps. A target is an objectName, or {"name": ..., "nth": 0, "text": ...,
// "where": {"property": ..., "equals": ...}}; {"text": "Number"} alone finds an item by what it says.
//   {"note": "text"}                                   a line in the log
//   {"wait": 300}                                      milliseconds
//   {"waitFor": {"target": t, "property": p, "equals": v, "timeout": 5000}}
//   {"click": t}   {"doubleClick": t}
//   {"type": "text", "into": t}                        click the item, replace its text by typing
//   {"type": "text"}                                   to whatever has keyboard focus
//   {"key": "Return", "modifiers": ["ctrl"]}           to whatever has keyboard focus; shortcuts are matched first
//   {"drop": {"on": t, "files": ["/path", ...]}}       files dragged in from a file manager
//   {"check": {"target": t, "property": p, "equals": v}}   also "contains", "atLeast"; logs PASS or FAIL
//   {"exists": t}   {"absent": t}                      these three are tried again for 1.5 s before they fail
//   {"shot": "name.png"}                               the whole window; fails when something is laid out beyond it
//   {"resize": [1200, 780]}   {"scheme": "dark"}
//   {"scroll": {"target": t, "y": 600}}                a scroll view or list, to a content position
//   {"tabWalk": 8, "first": t}                        press Tab eight times, log where focus lands; it must stay in view,
//                                                      and with "first" the first press must reach that item
//   {"tabTo": "name"}                                  press Tab until focus is on that item or inside it; it must be in
//                                                      view and show that it has the focus
//   {"set": {"target": t, "property": p, "value": v}}  for what has no control of its own
//   {"closeWindow": true}                              what the window's close button does
//   {"invoke": {"object": "backend.retrieve", "method": "copySelectedTo", "args": ["file:///tmp/x"]}}
//   {"invoke": {"target": t, "method": "importMetadata", "args": ["file:///tmp/x.json"]}}
//                                                      stand in for a native file dialog, which cannot be driven:
//                                                      they call what the dialog's "accepted" calls
//
// An item counts as there when it is shown and, for "exists", clicks and drops, in view (see UiProbe).
//
// The events stay inside this process. A script must still not click what reaches outside it: links that open the
// browser, "Show folder", "Copy" buttons that write to the clipboard.
class UiDriver : public QObject
{
    Q_OBJECT
public:
    UiDriver(QQuickWindow *window, Backend *backend, const QString &scriptFile, const QString &outputDir, QObject *parent = nullptr);

    void start();
    int failures() const { return m_failures; }

signals:
    void finished(int failures);

private:
    void next();
    void run(const QJsonObject &step);
    void settle();
    QImage grab();
    static QString describe(const QJsonValue &target);
    void log(const QString &line);
    void fail(const QString &line);
    QQuickItem *find(const QJsonValue &target, bool report = true);
    QQuickItem *findInView(const QJsonValue &target);
    static bool matches(const QVariant &actual, const QJsonObject &spec, QString *expected);
    void key(const QString &name, const QJsonArray &modifiers);
    void drop(QQuickItem *item, const QJsonArray &files);
    void invoke(const QJsonObject &spec);
    void waitFor(const QJsonObject &spec);
    void expect(const QJsonObject &step);
    void poll(int timeout, int interval, const std::function<bool(bool)> &attempt);

    QPointer<QQuickWindow> m_window;
    UiProbe m_ui;
    Backend *m_backend;
    QString m_outputDir;
    QJsonArray m_steps;
    int m_index = 0;
    int m_failures = 0;
    int m_shots = 0;
    QFile m_log;
    QElapsedTimer m_clock;
};
