#include "appmain.h"

#include "backend.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(SciCat_BackendPlugin)
Q_IMPORT_QML_PLUGIN(SciCat_UiPlugin)
Q_IMPORT_QML_PLUGIN(SciCat_AppPlugin)

namespace scicat {

int runApplication(int argc, char *argv[], const std::function<void(QQmlApplicationEngine &, Backend &)> &ready)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("PSI"));
    QGuiApplication::setApplicationName(QStringLiteral("SciCatQml"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("SciCat"));
    QGuiApplication::setApplicationVersion(QStringLiteral(SCICAT_VERSION));
#ifndef Q_OS_MACOS
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/SciCat/App/icons/scicat-logo.svg")));
#endif
    // the pages use Material attributes for colour and density, so the style is fixed
    QQuickStyle::setStyle(QStringLiteral("Material"));

    Backend backend(Backend::configFromEnvironment(QCoreApplication::applicationDirPath(), QStringLiteral(SCICAT_VERSION)));
    Backend::setInstance(&backend);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("SciCat.App", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    if (ready)
        ready(engine, backend);
    return app.exec();
}

} // namespace scicat
