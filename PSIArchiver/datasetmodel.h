#ifndef DATASETMODEL_H
#define DATASETMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <algorithm>

class DatasetModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatasetModel(const std::vector<QStringList>& datasetContainer, QObject *parent = nullptr) :
        QAbstractTableModel(parent), datasetContainerModel(datasetContainer){}
    DatasetModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant dataRow(const QModelIndex &index, int role);
    void setContainer(const std::vector<QStringList>& datasetContainer);
    void sort(int column, Qt::SortOrder order) override;
    void appendData(const std::vector<QStringList>& newDatasetContainer);
    void appendData(const QStringList newDatasetContainer);
    void emitDataChanged();

private:
    std::vector<QStringList> datasetContainerModel;
};



#endif // DATASETMODEL_H
