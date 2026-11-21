#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

// What scicat-cli datasetIngestor is fed: the metadata file and the file listing.
// Ported from DatasetIngestor::prepareIngestion() and the helpers in PSIArchiver/dialog.cpp.
namespace scicat {

struct DatasetDraft
{
    bool raw = false;
    QString datasetName;
    QString description;
    QString owner;
    QString ownerGroup;
    QString ownerEmail;            // left out of the file when empty: scicat-cli then uses the address of the token's owner
    QString proposalId;            // left out of the file when empty
    QString principalInvestigator; // raw data: an email address, or a name when the proposal has no usable address
    QString creationLocation;      // raw data
    QJsonObject scientificMetadata; // raw data
    QString investigator;          // derived data
    QString usedSoftware;          // derived data
    QStringList inputDatasets;     // derived data, dataset PIDs
    QStringList paths;             // absolute files and folders
};

// Deepest folder that contains every selected path's parent. For one entry, or entries that share a parent, this is
// that parent, like the Widgets GUI. For entries from different folders the Widgets GUI returns the parent of the
// first entry, which makes the listing of the others unusable; here they get their real common folder.
QString commonSourceFolder(const QStringList &paths);

// The entries relative to the source folder, one per line of the file listing.
QStringList fileListing(const QStringList &paths, const QString &sourceFolder);

QJsonObject metadataJson(const DatasetDraft &draft, const QString &sourceFolder);

// "my run 1" -> "my_run_1": dataset names carry no spaces
QString normalizedDatasetName(const QString &name);

// One entry: its base name. Several: the base name of their common folder.
QString guessDatasetName(const QStringList &paths);

// "/INSTITUTE[/FACILITY]/INSTRUMENT"; empty and *error set when a required part is missing
QString customCreationLocation(const QString &institute, const QString &facility, const QString &instrument, QString *error);

// Paths as scicat-cli expects them: unchanged, except "C:/data" -> "/C/data" on Windows.
QStringList cliPaths(const QStringList &paths);

struct IngestFiles
{
    QString directory;
    QString metadataFile;
    QString fileListingFile;
    QString error;

    bool ok() const { return error.isEmpty(); }
};

// Writes <logPath>/ingestion/<datasetName><epochSeconds>/{metadata,filelisting}.
IngestFiles writeIngestFiles(const DatasetDraft &draft, const QString &logPath, const QString &metadataFileName,
                             const QString &fileListingFileName, qint64 epochSeconds);

} // namespace scicat
