#pragma once

#include "apiclient.h"
#include "catalog.h"
#include "datasetlistmodel.h"
#include "metadatamodel.h"

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class FileListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by the backend")

public:
    enum Roles { PathRole = Qt::UserRole + 1, SizeTextRole, TimeTextRole };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return parent.isValid() ? 0 : int(m_files.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override { return {{PathRole, "path"}, {SizeTextRole, "sizeText"}, {TimeTextRole, "timeText"}}; }
    void setFiles(const QList<scicat::DataFile> &files);
    const QList<scicat::DataFile> &files() const { return m_files; }

private:
    QList<scicat::DataFile> m_files;
};

// Files and scientific metadata of the dataset the user is looking at, loaded on demand and remembered.
class DatasetDetails : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.retrieve.details")

    Q_PROPERTY(bool available READ available NOTIFY datasetChanged)
    Q_PROPERTY(QString pid READ pid NOTIFY datasetChanged)
    Q_PROPERTY(QString name READ name NOTIFY datasetChanged)
    Q_PROPERTY(QString type READ type NOTIFY datasetChanged)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY datasetChanged)
    Q_PROPERTY(QString createdText READ createdText NOTIFY datasetChanged)
    Q_PROPERTY(QString sourceFolder READ sourceFolder NOTIFY datasetChanged)
    Q_PROPERTY(int state READ state NOTIFY datasetChanged)
    Q_PROPERTY(QString jobNote READ jobNote NOTIFY datasetChanged)
    Q_PROPERTY(LoadState filesState READ filesState NOTIFY filesChanged)
    Q_PROPERTY(QString filesNote READ filesNote NOTIFY filesChanged)
    Q_PROPERTY(FileListModel *files READ files CONSTANT)
    Q_PROPERTY(LoadState metadataState READ metadataState NOTIFY metadataChanged)
    Q_PROPERTY(QString metadataNote READ metadataNote NOTIFY metadataChanged)
    Q_PROPERTY(MetadataModel *metadata READ metadata CONSTANT)

public:
    enum LoadState { Idle, Loading, Loaded, Failed };
    Q_ENUM(LoadState)

    explicit DatasetDetails(scicat::ApiClient *api, QObject *parent = nullptr);

    bool available() const { return !m_dataset.pid.isEmpty(); }
    QString pid() const { return m_dataset.pid; }
    QString name() const { return m_dataset.name; }
    QString type() const { return m_dataset.type; }
    QString sizeText() const;
    QString createdText() const;
    QString sourceFolder() const { return m_dataset.sourceFolder; }
    int state() const { return m_state; }
    QString jobNote() const { return m_jobNote; }
    LoadState filesState() const { return m_filesState; }
    QString filesNote() const { return m_filesNote; }
    FileListModel *files() const { return m_files; }
    LoadState metadataState() const { return m_metadataState; }
    QString metadataNote() const { return m_metadataNote; }
    MetadataModel *metadata() const { return m_metadata; }

    void show(const scicat::DatasetSummary &dataset, DatasetListModel::State state, const QString &jobNote);
    void updateState(DatasetListModel::State state, const QString &jobNote);
    void clear();
    void forget();
    Q_INVOKABLE void retry();

signals:
    void datasetChanged();
    void filesChanged();
    void metadataChanged();

private:
    void loadFiles();
    void loadMetadata();
    void applyFiles(const QList<scicat::DataFile> &files);

    scicat::ApiClient *m_api;
    scicat::DatasetSummary m_dataset;
    int m_state = 0;
    QString m_jobNote;
    FileListModel *m_files;
    MetadataModel *m_metadata;
    LoadState m_filesState = Idle;
    LoadState m_metadataState = Idle;
    QString m_filesNote;
    QString m_metadataNote;
    QHash<QString, QList<scicat::DataFile>> m_fileCache;
    QHash<QString, QJsonObject> m_metadataCache;
    QPointer<scicat::ApiReply> m_filesReply;
    QPointer<scicat::ApiReply> m_metadataReply;
};
