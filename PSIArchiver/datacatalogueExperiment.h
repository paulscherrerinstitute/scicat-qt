#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include "datacatalogueClasses.h"

namespace SciCat {
namespace DataCatalogueCore {


class Experiment
{
public:
    void setInvestigator(const QString& investigator);
    QString getInvestigator() const;


private:
    Experiment(DataCatalogue * const parent);
    friend class SciCat::DataCatalogue;
    DataCatalogue * const scicatBase;

    QString investigator;


};


}
}


#endif // EXPERIMENT_H
