#include "fileentrymodel.h"

#include "appsettings.h"
#include "datasetmetadata.h"
#include "formatting.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

using namespace scicat;

FileEntryModel::FileEntryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

FileEntryModel::~FileEntryModel()
{
    for (const Entry &entry : std::as_const(m_entries))
        entry.cancel->store(true);
}

int FileEntryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant FileEntryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case PathRole: return QDir::toNativeSeparators(entry.path);
    case NameRole: return QFileInfo(entry.path).fileName();
    case FolderRole: return QDir::toNativeSeparators(QFileInfo(entry.path).path());
    case IsFolderRole: return entry.scanning ? QFileInfo(entry.path).isDir() : entry.result.isFolder;
    case ExistsRole: return entry.scanning || entry.result.exists;
    case ScanningRole: return entry.scanning;
    case DetailRole:
        if (entry.scanning)
            return tr("Counting…");
        if (!entry.result.exists)
            return tr("Not found");
        if (!entry.result.isFolder)
            return formatBytes(entry.result.bytes);
        return entry.result.files == 1 ? tr("1 file · %1").arg(formatBytes(entry.result.bytes))
                                       : tr("%1 files · %2").arg(formatCount(entry.result.files), formatBytes(entry.result.bytes));
    }
    return QVariant();
}

QHash<int, QByteArray> FileEntryModel::roleNames() const
{
    return {{PathRole, "path"}, {NameRole, "name"}, {FolderRole, "folder"}, {IsFolderRole, "isFolder"},
            {ExistsRole, "exists"}, {ScanningRole, "scanning"}, {DetailRole, "detail"}};
}

bool FileEntryModel::scanning() const
{
    for (const Entry &entry : m_entries) {
        if (entry.scanning)
            return true;
    }
    return false;
}

qint64 FileEntryModel::totalFiles() const
{
    qint64 total = 0;
    for (const Entry &entry : m_entries)
        total += entry.result.files;
    return total;
}

qint64 FileEntryModel::totalBytes() const
{
    qint64 total = 0;
    for (const Entry &entry : m_entries)
        total += entry.result.bytes;
    return total;
}

int FileEntryModel::missingCount() const
{
    int missing = 0;
    for (const Entry &entry : m_entries)
        missing += !entry.scanning && !entry.result.exists;
    return missing;
}

bool FileEntryModel::overLimit() const
{
    // the Widgets GUI refuses a dataset unless it has fewer files than the limit
    return totalFiles() >= AppSettings::maxFilesPerDataset;
}

QString FileEntryModel::summary() const
{
    if (m_entries.isEmpty())
        return QString();
    const QString entries = m_entries.size() == 1 ? tr("1 entry") : tr("%1 entries").arg(m_entries.size());
    if (scanning())
        return tr("%1 · counting files…").arg(entries);
    const QString files = totalFiles() == 1 ? tr("1 file") : tr("%1 files").arg(formatCount(totalFiles()));
    return tr("%1 · %2 · %3").arg(entries, files, formatBytes(totalBytes()));
}

QString FileEntryModel::sourceFolder() const
{
    return m_entries.isEmpty() ? QString() : QDir::toNativeSeparators(commonSourceFolder(paths()));
}

QStringList FileEntryModel::paths() const
{
    QStringList result;
    for (const Entry &entry : m_entries)
        result << entry.path;
    return result;
}

int FileEntryModel::addUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    return addPaths(paths);
}

int FileEntryModel::addPaths(const QStringList &paths)
{
    QStringList fresh;
    const QStringList known = this->paths();
    for (const QString &raw : paths) {
        const QString path = QDir::cleanPath(raw);
        if (!path.isEmpty() && !known.contains(path) && !fresh.contains(path))
            fresh << path;
    }
    if (fresh.isEmpty())
        return 0;
    beginInsertRows(QModelIndex(), int(m_entries.size()), int(m_entries.size() + fresh.size()) - 1);
    for (const QString &path : std::as_const(fresh)) {
        Entry entry;
        entry.path = path;
        entry.cancel = std::make_shared<std::atomic_bool>(false);
        m_entries.append(entry);
    }
    endInsertRows();
    for (const QString &path : std::as_const(fresh)) {
        for (const Entry &entry : std::as_const(m_entries)) {
            if (entry.path == path)
                scan(path, entry.cancel);
        }
    }
    emit countChanged();
    emit totalsChanged();
    return int(fresh.size());
}

void FileEntryModel::removeAt(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_entries.at(row).cancel->store(true);
    m_entries.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit totalsChanged();
}

void FileEntryModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    for (const Entry &entry : std::as_const(m_entries))
        entry.cancel->store(true);
    m_entries.clear();
    endResetModel();
    emit countChanged();
    emit totalsChanged();
}

QStringList FileEntryModel::takeAll()
{
    const QStringList all = paths();
    clear();
    return all;
}

void FileEntryModel::scan(const QString &path, const std::shared_ptr<std::atomic_bool> &cancel)
{
    auto *watcher = new QFutureWatcher<ScanResult>(this);
    connect(watcher, &QFutureWatcher<ScanResult>::finished, this, [this, watcher, path, cancel] {
        watcher->deleteLater();
        if (cancel->load())
            return;
        for (int row = 0; row < m_entries.size(); ++row) {
            Entry &entry = m_entries[row];
            if (entry.path != path || entry.cancel != cancel)
                continue;
            entry.result = watcher->result();
            entry.scanning = false;
            emit dataChanged(index(row), index(row));
            emit totalsChanged();
            return;
        }
    });
    watcher->setFuture(QtConcurrent::run([path, cancel] { return scanPath(path, cancel.get()); }));
}
