#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QtQml/qqmlregistration.h>

// The raw datasets a derived dataset was computed from.
class InputDatasetModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.archive.inputDatasets")

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles { NameRole = Qt::UserRole + 1, PidRole };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override { return {{NameRole, "name"}, {PidRole, "pid"}}; }

    int count() const { return int(m_rows.size()); }
    QStringList pids() const;
    bool add(const QString &pid, const QString &name);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    struct Row
    {
        QString name;
        QString pid;
    };
    QList<Row> m_rows;
};
