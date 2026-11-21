#include "appmain.h"
#include "scenarioplayer.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStyleHints>
#include <cstdio>

// The application with a scenario of tests/parity played on it (see its README, "Driver protocol").
//
// It refuses to start without the environment of a test run, so that it can never talk to a real SciCat server or
// touch the user's own settings: every request goes to SCICAT_TEST_API, and the settings live next to
// SCICAT_TEST_SETTINGS.
int main(int argc, char *argv[])
{
    for (const char *name : {"SCICAT_TEST_SCENARIO", "SCICAT_TEST_OUT", "SCICAT_TEST_API", "SCICAT_TEST_SETTINGS"}) {
        if (qEnvironmentVariableIsEmpty(name)) {
            std::fprintf(stderr, "scenario driver: %s is not set; this program only runs under tests/parity/run_scenarios.py\n", name);
            return 2;
        }
    }
    // The scenario gives a settings file of the Widgets GUI and reads one back. This application keeps its own,
    // made from the given one, and writes the other for the comparison (ScenarioPlayer::exportSettings).
    const QString given = qEnvironmentVariable("SCICAT_TEST_SETTINGS");
    const QString own = given + QStringLiteral(".qml");
    ScenarioPlayer::importSettings(given, own);
    qputenv("SCICAT_QML_SETTINGS", own.toLocal8Bit());
    // one address per server, so that the fake SciCat can tell which server a request was meant for
    qputenv("SCICAT_QML_API", (qEnvironmentVariable("SCICAT_TEST_API") + QStringLiteral("/{server}/api/")).toLocal8Bit());
    qunsetenv("SCICAT_QML_HIDDEN");
    qunsetenv("SCICAT_WINDOW_TAG");

    return scicat::runApplication(argc, argv, [&given](QQmlApplicationEngine &engine, Backend &backend) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        // Tab reaches every control, as on Linux and Windows and on a Mac with "Keyboard navigation" switched on
        QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
        auto *player = new ScenarioPlayer(window, &backend, given, window);
        player->start();
    });
}
