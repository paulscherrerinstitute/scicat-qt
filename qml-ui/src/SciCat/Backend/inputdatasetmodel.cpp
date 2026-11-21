#include "inputdatasetmodel.h"

QVariant InputDatasetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();
    const Row &row = m_rows.at(index.row());
    return role == NameRole ? QVariant(row.name) : role == PidRole ? QVariant(row.pid) : QVariant();
}

QStringList InputDatasetModel::pids() const
{
    QStringList result;
    for (const Row &row : m_rows)
        result << row.pid;
    return result;
}

bool InputDatasetModel::add(const QString &pid, const QString &name)
{
    if (pid.isEmpty() || pids().contains(pid))
        return false;
    beginInsertRows(QModelIndex(), int(m_rows.size()), int(m_rows.size()));
    m_rows.append({name, pid});
    endInsertRows();
    emit countChanged();
    return true;
}

void InputDatasetModel::removeAt(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_rows.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void InputDatasetModel::clear()
{
    if (m_rows.isEmpty())
        return;
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit countChanged();
}
