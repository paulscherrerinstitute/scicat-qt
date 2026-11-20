#include <QtTest>

#include "datacatalogue.h"
#include "datacatalogueExperiment.h"

// defined in main.cpp, which the test does not link
QVersionNumber currentVersionNumber(0, 4);

class TestExperiment : public QObject
{
    Q_OBJECT

private slots:
    void editedOwnerGroupKeepsPgroupFilter();
    void choosingPgroupResetsOwnerGroup();
};

// the owner group edited for an ingest must not move the retrieve list, the dataset browser or retrieve jobs
void TestExperiment::editedOwnerGroupKeepsPgroupFilter()
{
    SciCat::DataCatalogue catalogue(nullptr, 1);
    auto& experiment = *catalogue.experiment();
    experiment.setPgroup("p12345");
    experiment.setOwnerGroup("p22222");

    QCOMPARE(experiment.getOwnerGroup(), QString("p22222"));
    QCOMPARE(experiment.getPgroup(), QString("p12345"));
    QCOMPARE(experiment.pgroupWhere(), QJsonObject({{"ownerGroup", "p12345"}}));
}

void TestExperiment::choosingPgroupResetsOwnerGroup()
{
    SciCat::DataCatalogue catalogue(nullptr, 1);
    auto& experiment = *catalogue.experiment();
    experiment.setPgroup("p12345");
    experiment.setOwnerGroup("p22222");
    experiment.setPgroup("p33333");

    QCOMPARE(experiment.getOwnerGroup(), QString("p33333"));
    QCOMPARE(experiment.pgroupWhere(), QJsonObject({{"ownerGroup", "p33333"}}));
}

QTEST_GUILESS_MAIN(TestExperiment)
#include "tst_experiment.moc"
