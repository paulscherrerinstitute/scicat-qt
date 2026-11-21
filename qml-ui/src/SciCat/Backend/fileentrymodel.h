#pragma once

#include "filescan.h"

#include <QAbstractListModel>
#include <QList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

// The files and folders chosen for a dataset, each counted in the background.
class FileEntryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.archive.files")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY totalsChanged)
    Q_PROPERTY(qint64 totalFiles READ totalFiles NOTIFY totalsChanged)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY totalsChanged)
    Q_PROPERTY(int missingCount READ missingCount NOTIFY totalsChanged)
    Q_PROPERTY(bool overLimit READ overLimit NOTIFY totalsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY totalsChanged)
    Q_PROPERTY(QString sourceFolder READ sourceFolder NOTIFY countChanged)

public:
    enum Roles { PathRole = Qt::UserRole + 1, NameRole, FolderRole, IsFolderRole, ExistsRole, ScanningRole, DetailRole };

    explicit FileEntryModel(QObject *parent = nullptr);
    ~FileEntryModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_entries.size()); }
    bool scanning() const;
    qint64 totalFiles() const;
    qint64 totalBytes() const;
    int missingCount() const;
    bool overLimit() const;
    QString summary() const;
    QString sourceFolder() const;
    QStringList paths() const;

    // returns how many entries were new
    Q_INVOKABLE int addUrls(const QList<QUrl> &urls);
    Q_INVOKABLE int addPaths(const QStringList &paths);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void clear();
    // empties the list and hands back what was in it, for an undo
    Q_INVOKABLE QStringList takeAll();

signals:
    void countChanged();
    void totalsChanged();

private:
    struct Entry
    {
        QString path;
        scicat::ScanResult result;
        bool scanning = true;
        std::shared_ptr<std::atomic_bool> cancel;
    };
    void scan(const QString &path, const std::shared_ptr<std::atomic_bool> &cancel);

    QList<Entry> m_entries;
};
