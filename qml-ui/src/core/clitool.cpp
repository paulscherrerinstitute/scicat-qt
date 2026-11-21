#include "clitool.h"

#include "servers.h"

#include <QDir>
#include <QStandardPaths>

namespace scicat {

QString locateCli(const QString &applicationDirPath)
{
    const QDir appDir(applicationDirPath);
    QString cli = QStandardPaths::findExecutable(QStringLiteral("scicat-cli"),
                                                 {appDir.path(), appDir.filePath(QStringLiteral("../libexec/scicat"))});
    if (cli.isEmpty())
        cli = QStandardPaths::findExecutable(QStringLiteral("scicat-cli"));
    return cli;
}

QStringList ingestArguments(bool ingest, bool autoArchive, int serverId, const QString &token, const QString &metadataFile,
                            const QString &fileListingFile)
{
    QStringList arguments{QStringLiteral("datasetIngestor")};
    if (ingest)
        arguments << QStringLiteral("--ingest");
    if (autoArchive)
        arguments << QStringLiteral("--autoarchive");
    arguments << Servers::at(serverId).cliArguments;
    arguments << QStringLiteral("--noninteractive") << QStringLiteral("--allowexistingsource");
    arguments << QStringLiteral("--token") << token;
    arguments << metadataFile << fileListingFile;
    return arguments;
}

QStringList retrieveArguments(int serverId, const QString &pid, const QString &token, const QString &targetFolder)
{
    QStringList arguments{QStringLiteral("datasetRetriever")};
    arguments << Servers::at(serverId).cliArguments;
    arguments << QStringLiteral("--dataset") << pid;
    arguments << QStringLiteral("--token") << token;
    arguments << QStringLiteral("--retrieve") << targetFolder;
    return arguments;
}

QString displayCommand(const QString &program, const QStringList &arguments)
{
    QStringList shown{program};
    for (int i = 0; i < arguments.size(); ++i) {
        const bool secret = i > 0 && arguments.at(i - 1) == QLatin1String("--token");
        const QString argument = secret ? QStringLiteral("<hidden>") : arguments.at(i);
        shown << (argument.contains(QLatin1Char(' ')) ? QLatin1Char('"') + argument + QLatin1Char('"') : argument);
    }
    return shown.join(QLatin1Char(' '));
}

} // namespace scicat
