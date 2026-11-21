#include "appmain.h"
#include "uidriver.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QQmlApplicationEngine>

// The application with a script driving it:  SCICAT_E2E_SCRIPT=<walk.json> SCICAT_E2E_OUT=<folder> scicat_qml_e2e
// The exit code is the number of failed steps.
int main(int argc, char *argv[])
{
    return scicat::runApplication(argc, argv, [](QQmlApplicationEngine &engine, Backend &backend) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        // Tab reaches every control, as on Linux and Windows and on a Mac with "Keyboard navigation" switched on
        QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
        if (qEnvironmentVariableIsSet("SCICAT_E2E_GHOST")) {
            // A real window of the platform with its real render loop, that nobody sees or can click: fully
            // transparent, never active, and mouse clicks pass through it. The application was started hidden
            // (SCICAT_QML_HIDDEN), so nothing flashes up before this.
            window->setFlags(window->flags() | Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
            window->setOpacity(0);
            window->show();
        }
        auto *driver = new UiDriver(window, &backend, qEnvironmentVariable("SCICAT_E2E_SCRIPT"), qEnvironmentVariable("SCICAT_E2E_OUT"), window);
        QObject::connect(driver, &UiDriver::finished, qApp, [](int failures) { QCoreApplication::exit(qMin(failures, 100)); });
        driver->start();
    });
}
