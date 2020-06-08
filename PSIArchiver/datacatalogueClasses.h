#ifndef DATACATALOGUECLASSES_H
#define DATACATALOGUECLASSES_H

#include <QString>
#include <QStringList>
#include <QJsonDocument>

namespace SciCat {

class DataCatalogue;

enum class authenticationState {Valid, Invalid, ConnectionError};
enum class authenticationStateAccount {ValidUser, ValidBLAccount, Invalid, ConnectionError};

enum class serverType {Production, QA, Development};

namespace DataCatalogueCore {

class Experiment;
class User;

}

}

#endif // DATACATALOGUECLASSES_H
