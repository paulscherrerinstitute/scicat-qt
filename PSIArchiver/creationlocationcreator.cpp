#include "creationlocationcreator.h"

CreationLocationCreator::CreationLocationCreator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::creationLocationCreator)
{
    ui->setupUi(this);

    connect(ui->pushButtonAddEntry, SIGNAL(clicked()), this, SLOT(addEntry()));

    baseUi = dynamic_cast<Dialog*>(parent);
}

void CreationLocationCreator::addEntry(){
    QStringList creationLocationSep;
    if (ui->lineEditInstitute->text().length()==0){
        QMessageBox::information(this, tr("Error"), tr("Institute cannot be empty") );
        return;
    } else {
        creationLocationSep.push_back(ui->lineEditInstitute->text().toUpper());
    }

    if (ui->lineEditFacility->text().length()>0){
        creationLocationSep.push_back(ui->lineEditFacility->text().toUpper());
    }

    if ((ui->lineEditInstrument->text().length()==0)){
        QMessageBox::information(this, tr("Error"), tr("Instrument cannot be empty") );
        return;
    } else {
        creationLocationSep.push_back(ui->lineEditInstrument->text().toUpper());
    }

    qDebugLine("CreationLocation") << creationLocationSep.join("/");
    baseUi->ui->comboBoxCreationLocation->addItem("/"+creationLocationSep.join("/"));
    baseUi->ui->comboBoxCreationLocation->setCurrentIndex(baseUi->ui->comboBoxCreationLocation->count()-1);

    close();

}

CreationLocationCreator::~CreationLocationCreator()
{
    delete ui;
}
