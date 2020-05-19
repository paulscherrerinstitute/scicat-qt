#ifndef FILEDIALOGFILELISTING_H
#define FILEDIALOGFILELISTING_H
#include "dialog.h"


class FileDialogFilelisting : public QFileDialog{
    Q_OBJECT

public:
    explicit FileDialogFilelisting(QWidget *parent);
    QStringList selectedFiles();
    bool eventFilter(QObject* watched, QEvent* event);
    bool removeDuplicates = true;

public slots:
    void chooseClicked();
private:
    QListView *m_listView;
    QTreeView *m_treeView;
    QPushButton *m_btnOpen;
    QStringList m_selectedFiles;
    void appendEntry();
    Dialog *baseUi;
};

#endif // FILEDIALOGFILELISTING_H
