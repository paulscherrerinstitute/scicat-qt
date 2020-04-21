#include "datasetmodel.h"
#include <iostream>


int DatasetModel::rowCount(const QModelIndex &parent) const {
    return datasetContainerModel.size();
}
int DatasetModel::columnCount(const QModelIndex &parent) const {
    return 2;
}

void DatasetModel::setContainer(const std::vector<QStringList>& datasetContainer){
    datasetContainerModel = std::vector<QStringList>(datasetContainer);
}

QVariant DatasetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()){
        return QVariant();
    }
    if (index.row() >= (int)datasetContainerModel.size()){
        return QVariant();
    }
    if (index.column() >= 2){
        return QVariant();
    }
    if (role == Qt::DisplayRole || role==Qt::EditRole){
        return datasetContainerModel[index.row()].at(index.column());
    } else {
        return QVariant();
    }
}

QVariant DatasetModel::dataRow(const QModelIndex &index, int role)
{
    if (!index.isValid()){
        return QVariant();
    }
    if (index.row() >= (int)datasetContainerModel.size()){
        return QVariant();
    }
    if (index.column() >= 2){
        return QVariant();
    }
    if (role == Qt::DisplayRole){
        return datasetContainerModel[index.row()];
    } else {
        return QVariant();
    }
}


void DatasetModel::sort(int column, Qt::SortOrder order) {
    struct customCompare {
        customCompare(int column, Qt::SortOrder order){
            this->column = column;
            this->order = order;
        }
        int column;
        Qt::SortOrder order;

        bool operator()(QStringList&a, QStringList&b) const
        {
            switch(order){
            case Qt::SortOrder::AscendingOrder:
                return a.at(column)<b.at(column);
            case Qt::SortOrder::DescendingOrder:
                //something
                return a.at(column)>b.at(column);
            }
        }
    };
    std::sort(datasetContainerModel.begin(), datasetContainerModel.end(), customCompare(column, order));
    emit dataChanged(QModelIndex(), QModelIndex());
}

void DatasetModel::appendData(const std::vector<QStringList>& newDatasetContainer){
    for (auto v:newDatasetContainer){
        datasetContainerModel.push_back(v);
    }
    QModelIndex top = createIndex(0,0,nullptr);
    QModelIndex bottom = createIndex(datasetContainerModel.size(),1,nullptr);
    emit dataChanged(top, bottom);
}

void DatasetModel::appendData(const QStringList newDatasetContainer){
    datasetContainerModel.push_back(newDatasetContainer);
    QModelIndex top = createIndex(0,0,nullptr);
    QModelIndex bottom = createIndex(datasetContainerModel.size(),1,nullptr);
    emit dataChanged(top, bottom);
}

void DatasetModel::emitDataChanged(){
    QModelIndex top = createIndex(0,0,nullptr);
    QModelIndex bottom = createIndex(datasetContainerModel.size(),1,nullptr);
    emit dataChanged(top, bottom);
}
