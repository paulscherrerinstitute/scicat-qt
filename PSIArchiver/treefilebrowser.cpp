#include "treefilebrowser.h"


TreeFileBrowser::TreeFileBrowser(QFileSystemModel* model, const QString& rootAbsPath, QObject* parent):
        QSortFilterProxyModel(parent),
        fsm(model)
{
        setSourceModel(fsm);
        if(!rootAbsPath.isEmpty()) fsm->setRootPath(rootAbsPath);
}

QModelIndex TreeFileBrowser::mapFromSource(const QModelIndex& sourceIndex) const
{
        if(!sourceIndex.isValid() || sourceIndex.row() < 0) return QModelIndex();

        return ptrToProxyIdx[sourceIndex.internalPointer()];
}

QModelIndex TreeFileBrowser::mapToSource(const QModelIndex& proxyIndex) const
{
        if(!proxyIndex.isValid()) return badSrcIndex();

        if(!ptrToSrcIdx.contains(proxyIndex.internalPointer())) {return badSrcIndex();}

        return ptrToSrcIdx[proxyIndex.internalPointer()];
}

QModelIndex TreeFileBrowser::buddy(const QModelIndex& index) const
{
        if(!index.isValid()) return QModelIndex();

        return mapFromSource(fsModel()->buddy(mapToSource(index)));
}

QModelIndex TreeFileBrowser::index(int row, int column, const QModelIndex& parent) const
{
        if(column < 0 || column > 1) return QModelIndex();

        QModelIndex sourceIndex;
        if(!parent.isValid()) {sourceIndex = rootIndex(0);}
        else {sourceIndex = fsModel()->index(row, column, mapToSource(parent));}
        void* const ptr  = sourceIndex.internalPointer();
        ptrToSrcIdx[ptr] = sourceIndex;
        ptrToProxyIdx[ptr] = createIndex(row, column, ptr);

        return ptrToProxyIdx[ptr];
}

QModelIndex TreeFileBrowser::parent(const QModelIndex& idx) const
{
        if(!idx.isValid()) return QModelIndex();

        if(idx.internalPointer() == rootIndex(idx.column()).internalPointer())
        {
                return QModelIndex();
        }

        return mapFromSource(fsModel()->parent(mapToSource(idx)));
}

bool TreeFileBrowser::hasChildren(const QModelIndex& parent) const
{
        if(!parent.isValid()) return true;

        return fsModel()->hasChildren(mapToSource(parent));
}

int TreeFileBrowser::columnCount(const QModelIndex& parent) const
{
        if(!parent.isValid()) return 1;
        return fsModel()->columnCount(mapToSource(parent));
}

int TreeFileBrowser::rowCount(const QModelIndex& parent) const
{
        if(!parent.isValid()) return 1;

        if(fsModel()->fileInfo(mapToSource(parent)).isDir() && fsModel()->rowCount(mapToSource(parent)) == 0)
        {
                if(canFetchMore(parent)) const_cast<TreeFileBrowser*>(this)->fetchMore(parent);;
        }

        return fsModel()->rowCount(mapToSource(parent));
}

QVariant TreeFileBrowser::data(const QModelIndex& index, int role) const
{
        if(role == Qt::CheckStateRole)
        {
                if(ptrToChecked[index.internalPointer()] ) return Qt::Checked;
                else return Qt::Unchecked;
        }

        return fsModel()->data(mapToSource(index), role);
}

bool TreeFileBrowser::setData(const QModelIndex& index, const QVariant& value, int role)
{
        if (role == Qt::CheckStateRole && index.column() == 0)
        {
                ptrToChecked[index.internalPointer()] = static_cast<Qt::CheckState>(value.toInt()); //!ptrToChecked[index.internalPointer()];

                emit dataChanged(index, index);
                return true;
        }
        return  fsModel()->setData(mapToSource(index), value, role);
}

Qt::ItemFlags TreeFileBrowser::flags(const QModelIndex& index) const
{
        return fsModel()->flags(mapToSource(index)) | Qt::ItemIsUserCheckable;
}

QMimeData* TreeFileBrowser::mimeData(const QModelIndexList& indexes) const
{
        if(indexes.isEmpty()) return fsModel()->mimeData(indexes);

        QModelIndexList srcIndexes;

        for(int i = 0; i < indexes.count(); ++i)
        {
                srcIndexes.append(mapToSource(indexes[i]));
        }

        return fsModel()->mimeData(srcIndexes);
}

QStringList TreeFileBrowser::mimeTypes() const
{
        return fsModel()->mimeTypes();
}

QVariant TreeFileBrowser::headerData(int section, Qt::Orientation orientation, int role) const
{
        return fsModel()->headerData(section, orientation, role);
}

bool TreeFileBrowser::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
        return fsModel()->setHeaderData(section, orientation, value, role);
}

QList<QFileInfo> TreeFileBrowser::checkedData(bool checked, int role) const
{
        Q_UNUSED(role);

        QMap<void*, bool>::const_iterator it = ptrToChecked.begin();
        QList<QFileInfo> dataList;
        for(; it != ptrToChecked.end(); ++it)
        {
                if(it.value() != checked) continue;
                dataList.append(fsModel()->fileInfo(ptrToSrcIdx[it.key()]));
        }

        return dataList;
}

void TreeFileBrowser::fetchMore(const QModelIndex& parent)
{
        fsModel()->fetchMore(mapToSource(parent));
}

bool TreeFileBrowser::canFetchMore(const QModelIndex& parent) const
{
        return fsModel()->canFetchMore(mapToSource(parent));
}

QModelIndex TreeFileBrowser::sibling(int row, int column, const QModelIndex& index) const
{
        if(!index.isValid()) return QModelIndex();

        return fsModel()->sibling(row, column, mapToSource(index));;
}

const QFileSystemModel* TreeFileBrowser::fsModel() const
{
        return fsm;
}

QFileSystemModel* TreeFileBrowser::fsModel()
{
        return fsm;
}

QModelIndex TreeFileBrowser::rootIndex(int column) const
{
        return fsModel()->index(fsModel()->rootPath(), column);
}

QModelIndex TreeFileBrowser::badSrcIndex() const
{
        return fsModel()->index(-1, -1);
}



