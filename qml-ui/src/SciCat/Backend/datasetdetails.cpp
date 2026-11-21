#include "datasetdetails.h"

#include "formatting.h"

#include <algorithm>

using namespace scicat;

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.size())
        return QVariant();
    const DataFile &file = m_files.at(index.row());
    switch (role) {
    case PathRole: return file.path;
    case SizeTextRole: return formatBytes(file.sizeBytes);
    case TimeTextRole: return formatDateTime(file.time);
    }
    return QVariant();
}

void FileListModel::setFiles(const QList<DataFile> &files)
{
    beginResetModel();
    m_files = files;
    std::sort(m_files.begin(), m_files.end(), [](const DataFile &a, const DataFile &b) { return a.path < b.path; });
    endResetModel();
}

DatasetDetails::DatasetDetails(ApiClient *api, QObject *parent)
    : QObject(parent)
    , m_api(api)
    , m_files(new FileListModel(this))
    , m_metadata(new MetadataModel(this))
{
}

QString DatasetDetails::sizeText() const
{
    return formatBytes(m_dataset.sizeBytes);
}

QString DatasetDetails::createdText() const
{
    return formatDateTime(m_dataset.created);
}

void DatasetDetails::show(const DatasetSummary &dataset, DatasetListModel::State state, const QString &jobNote)
{
    if (dataset.pid == m_dataset.pid) {
        updateState(state, jobNote);
        return;
    }
    m_dataset = dataset;
    m_state = state;
    m_jobNote = jobNote;
    emit datasetChanged();
    loadFiles();
    loadMetadata();
}

void DatasetDetails::updateState(DatasetListModel::State state, const QString &jobNote)
{
    if (m_state == state && m_jobNote == jobNote)
        return;
    m_state = state;
    m_jobNote = jobNote;
    emit datasetChanged();
}

void DatasetDetails::clear()
{
    if (!available())
        return;
    m_dataset = DatasetSummary();
    m_files->setFiles({});
    m_metadata->clear();
    m_filesState = Idle;
    m_metadataState = Idle;
    emit datasetChanged();
    emit filesChanged();
    emit metadataChanged();
}

void DatasetDetails::forget()
{
    m_fileCache.clear();
    m_metadataCache.clear();
    clear();
}

void DatasetDetails::retry()
{
    if (!available())
        return;
    if (m_filesState == Failed)
        loadFiles();
    if (m_metadataState == Failed)
        loadMetadata();
}

void DatasetDetails::loadFiles()
{
    if (m_filesReply) {
        disconnect(m_filesReply, nullptr, this, nullptr);
        m_filesReply->abort();
    }
    const QString pid = m_dataset.pid;
    if (m_fileCache.contains(pid)) {
        applyFiles(m_fileCache.value(pid));
        return;
    }
    m_files->setFiles({});
    m_filesState = Loading;
    m_filesNote.clear();
    emit filesChanged();
    m_filesReply = m_api->get(m_api->request(QStringLiteral("datasets"), ApiVersion::V3, datasetFilesFilter(pid)));
    connect(m_filesReply, &ApiReply::finished, this, [this, pid](ApiReply *answer) {
        if (pid != m_dataset.pid)
            return;
        bool ok = false;
        const QList<DataFile> files = parseDatasetFiles(answer->json(), &ok);
        if (!answer->ok() || !ok) {
            m_filesState = Failed;
            m_filesNote = tr("Could not load the files of this dataset from SciCat.");
            emit filesChanged();
            return;
        }
        m_fileCache.insert(pid, files);
        applyFiles(files);
    });
}

void DatasetDetails::applyFiles(const QList<DataFile> &files)
{
    m_files->setFiles(files);
    qint64 bytes = 0;
    for (const DataFile &file : files)
        bytes += file.sizeBytes;
    m_filesState = Loaded;
    m_filesNote = files.isEmpty() ? tr("SciCat lists no files for this dataset.")
                  : files.size() == 1 ? tr("1 file · %1").arg(formatBytes(bytes))
                                      : tr("%1 files · %2").arg(formatCount(files.size()), formatBytes(bytes));
    emit filesChanged();
}

void DatasetDetails::loadMetadata()
{
    if (m_metadataReply) {
        disconnect(m_metadataReply, nullptr, this, nullptr);
        m_metadataReply->abort();
    }
    const QString pid = m_dataset.pid;
    const auto apply = [this](const QJsonObject &metadata) {
        m_metadata->setJson(metadata);
        m_metadataState = Loaded;
        m_metadataNote = metadata.isEmpty() ? tr("This dataset has no scientific metadata.") : QString();
        emit metadataChanged();
    };
    if (m_metadataCache.contains(pid)) {
        apply(m_metadataCache.value(pid));
        return;
    }
    m_metadata->clear();
    m_metadataState = Loading;
    m_metadataNote.clear();
    emit metadataChanged();
    const QString endpoint = QStringLiteral("datasets/") + QString::fromUtf8(QUrl::toPercentEncoding(pid));
    m_metadataReply = m_api->get(m_api->request(endpoint, ApiVersion::V3, scientificMetadataFilter()));
    connect(m_metadataReply, &ApiReply::finished, this, [this, pid, apply](ApiReply *answer) {
        if (pid != m_dataset.pid)
            return;
        if (!answer->ok() || !answer->json().isObject()) {
            m_metadataState = Failed;
            m_metadataNote = tr("Could not load the scientific metadata of this dataset from SciCat.");
            emit metadataChanged();
            return;
        }
        const QJsonObject metadata = answer->json().object().value(QLatin1String("scientificMetadata")).toObject();
        m_metadataCache.insert(pid, metadata);
        apply(metadata);
    });
}
