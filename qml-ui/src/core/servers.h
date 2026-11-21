#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace scicat {

enum class ApiVersion { V3, V4 };

struct Server
{
    enum class Kind { Production, QA, Development };

    Kind kind = Kind::Production;
    QString name;
    QString apiBase;
    QString frontend;
    QString retrieveHost;
    QStringList cliArguments; // environment flags of scicat-cli for this server
};

// The SciCat instances the GUI knows, in the order of the stored "serverID" setting.
class Servers
{
public:
    static const QList<Server> &all();
    static QStringList names();
    // the ID comes from the persistent settings, which may be stale or hand-edited
    static int validId(int id);
    static const Server &at(int id);
    static QString tokenUrl(int id);
};

} // namespace scicat
