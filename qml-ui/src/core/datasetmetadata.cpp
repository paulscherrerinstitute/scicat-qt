#include "datasetmetadata.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

namespace scicat {

namespace {
QString parentFolder(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    if (slash < 0)
        return QString();
    return slash == 0 ? QStringLiteral("/") : path.left(slash);
}
} // namespace

QString commonSourceFolder(const QStringList &paths)
{
    if (paths.isEmpty())
        return QString();
    QStringList common = parentFolder(paths.first()).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString &path : paths) {
        const QStringList parts = parentFolder(path).split(QLatin1Char('/'), Qt::SkipEmptyParts);
        int shared = 0;
        while (shared < common.size() && shared < parts.size() && common.at(shared) == parts.at(shared))
            ++shared;
        common = common.mid(0, shared);
    }
    return QLatin1Char('/') + common.join(QLatin1Char('/'));
}

QStringList fileListing(const QStringList &paths, const QString &sourceFolder)
{
    QStringList entries;
    const QString prefix = sourceFolder.endsWith(QLatin1Char('/')) ? sourceFolder : sourceFolder + QLatin1Char('/');
    for (const QString &path : paths) {
        QString entry = path.startsWith(prefix) ? path.mid(prefix.size()) : path;
        while (entry.startsWith(QLatin1Char('/')))
            entry.remove(0, 1);
        entries << entry;
    }
    return entries;
}

QJsonObject metadataJson(const DatasetDraft &draft, const QString &sourceFolder)
{
    QJsonObject metadata;
    metadata[QLatin1String("sourceFolder")] = sourceFolder;
    metadata[QLatin1String("owner")] = draft.owner;
    metadata[QLatin1String("ownerGroup")] = draft.ownerGroup;
    if (!draft.ownerEmail.isEmpty())
        metadata[QLatin1String("ownerEmail")] = draft.ownerEmail;
    metadata[QLatin1String("description")] = draft.description;
    metadata[QLatin1String("datasetName")] = draft.datasetName;
    if (!draft.proposalId.isEmpty())
        metadata[QLatin1String("proposalId")] = draft.proposalId;

    if (draft.raw) {
        metadata[QLatin1String("principalInvestigator")] = draft.principalInvestigator;
        metadata[QLatin1String("creationLocation")] = draft.creationLocation;
        metadata[QLatin1String("type")] = QStringLiteral("raw");
        metadata[QLatin1String("scientificMetadata")] = draft.scientificMetadata;
    } else {
        metadata[QLatin1String("type")] = QStringLiteral("derived");
        metadata[QLatin1String("investigator")] = draft.investigator;
        metadata[QLatin1String("inputDatasets")] = QJsonArray::fromStringList(
            draft.inputDatasets.isEmpty() ? QStringList{QStringLiteral("N/A")} : draft.inputDatasets);
        metadata[QLatin1String("usedSoftware")] = QJsonArray::fromStringList(
            QStringList{draft.usedSoftware.isEmpty() ? QStringLiteral("N/A") : draft.usedSoftware});
    }
    return metadata;
}

QString normalizedDatasetName(const QString &name)
{
    return QString(name).replace(QLatin1Char(' '), QLatin1Char('_'));
}

QString guessDatasetName(const QStringList &paths)
{
    if (paths.isEmpty())
        return QString();
    if (paths.size() == 1)
        return QFileInfo(paths.first()).baseName();
    return QFileInfo(commonSourceFolder(paths)).baseName();
}

QString customCreationLocation(const QString &institute, const QString &facility, const QString &instrument, QString *error)
{
    QStringList parts;
    if (institute.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("institute");
        return QString();
    }
    parts << institute.trimmed().toUpper();
    if (!facility.trimmed().isEmpty())
        parts << facility.trimmed().toUpper();
    if (instrument.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("instrument");
        return QString();
    }
    parts << instrument.trimmed().toUpper();
    return QLatin1Char('/') + parts.join(QLatin1Char('/'));
}

QStringList cliPaths(const QStringList &paths)
{
#ifdef Q_OS_WIN
    QStringList converted;
    for (QString path : paths)
        converted << QLatin1Char('/') + path.replace(QLatin1Char(':'), QString());
    return converted;
#else
    return paths;
#endif
}

IngestFiles writeIngestFiles(const DatasetDraft &draft, const QString &logPath, const QString &metadataFileName,
                             const QString &fileListingFileName, qint64 epochSeconds)
{
    IngestFiles files;
    files.directory = logPath + QStringLiteral("/ingestion/") + draft.datasetName + QString::number(epochSeconds);
    if (!QDir().mkpath(files.directory)) {
        files.error = QStringLiteral("Could not create the folder %1.").arg(QDir::toNativeSeparators(files.directory));
        return files;
    }

    const QStringList paths = cliPaths(draft.paths);
    const QString sourceFolder = commonSourceFolder(paths);

    files.metadataFile = QDir(files.directory).filePath(metadataFileName);
    QFile metadataFile(files.metadataFile);
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        files.error = QStringLiteral("Could not write %1: %2").arg(QDir::toNativeSeparators(files.metadataFile), metadataFile.errorString());
        return files;
    }
    metadataFile.write(QJsonDocument(metadataJson(draft, sourceFolder)).toJson());
    metadataFile.close();

    files.fileListingFile = QDir(files.directory).filePath(fileListingFileName);
    QFile listingFile(files.fileListingFile);
    if (!listingFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        files.error = QStringLiteral("Could not write %1: %2").arg(QDir::toNativeSeparators(files.fileListingFile), listingFile.errorString());
        return files;
    }
    QTextStream out(&listingFile);
    const QStringList entries = fileListing(paths, sourceFolder);
    for (const QString &entry : entries)
        out << entry << "\n";
    return files;
}

} // namespace scicat
