#include "servers.h"

#include <QDebug>

namespace scicat {

const QList<Server> &Servers::all()
{
    static const QList<Server> servers{
        {Server::Kind::Production, QStringLiteral("Production"), QStringLiteral("https://dacat.psi.ch/api/"),
         QStringLiteral("https://discovery.psi.ch"), QStringLiteral("pb-retrieve.psi.ch"), {}},
        {Server::Kind::QA, QStringLiteral("QA"), QStringLiteral("https://dacat-qa.psi.ch/api/"),
         QStringLiteral("https://discovery-qa.psi.ch"), QStringLiteral("pbt-retrieve.psi.ch"), {QStringLiteral("--testenv")}},
        {Server::Kind::Development, QStringLiteral("Development"), QStringLiteral("https://scicat.development.psi.ch/api/"),
         QStringLiteral("https://discovery.development.psi.ch"), QStringLiteral("arematest2in.psi.ch"), {QStringLiteral("--devenv")}},
    };
    return servers;
}

QStringList Servers::names()
{
    QStringList result;
    for (const Server &server : all())
        result << server.name;
    return result;
}

int Servers::validId(int id)
{
    if (id < 0 || id >= all().size()) {
        qWarning() << "Invalid server ID" << id << "- falling back to Production";
        return 0;
    }
    return id;
}

const Server &Servers::at(int id)
{
    return all().at(validId(id));
}

QString Servers::tokenUrl(int id)
{
    return at(id).frontend + QStringLiteral("/user");
}

} // namespace scicat
