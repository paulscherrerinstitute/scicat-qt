#ifndef CREATIONLOCATIONCREATOR_H
#define CREATIONLOCATIONCREATOR_H

#include "dialog.h"
#include "ui_creationlocationcreator.h"


class CreationLocationCreator : public QDialog
{
    Q_OBJECT

public:
    explicit CreationLocationCreator(QWidget *parent = nullptr);
    ~CreationLocationCreator();
private slots:
    void addEntry();
private:
    Ui::creationLocationCreator *ui;
    Dialog* baseUi;
};

#endif // CREATIONLOCATIONCREATOR_H
