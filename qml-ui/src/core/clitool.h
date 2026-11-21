#pragma once

#include <QString>
#include <QStringList>

// Finding and calling scicat-cli. Ported from Login::updateCLITools(), DatasetIngestor::start() and
// DatasetRetriever::start() of the Widgets GUI: same commands, same arguments, same order, on every platform.
namespace scicat {

// The scicat-cli bundled with the GUI, then the one on PATH; empty when neither exists.
QString locateCli(const QString &applicationDirPath);

QStringList ingestArguments(bool ingest, bool autoArchive, int serverId, const QString &token, const QString &metadataFile,
                            const QString &fileListingFile);

QStringList retrieveArguments(int serverId, const QString &pid, const QString &token, const QString &targetFolder);

// The command as text for the log, with the token replaced by a placeholder.
QString displayCommand(const QString &program, const QStringList &arguments);

} // namespace scicat
