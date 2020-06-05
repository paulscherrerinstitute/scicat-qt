#include "datacatalogueExperiment.h"

using namespace SciCat;


void DataCatalogueCore::Experiment::setInvestigator(const QString &inv)
{
    investigator = inv;
}

QString DataCatalogueCore::Experiment::getInvestigator() const
{
    return investigator;
}

DataCatalogueCore::Experiment::Experiment(DataCatalogue * const parent) : scicatBase(parent)
{

}
