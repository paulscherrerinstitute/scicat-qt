#include "dialog.h"
#include "login.h"
#include <QApplication>
#include <QCoreApplication>

QVersionNumber currentVersionNumber(0,4);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifndef Q_OS_MACOS
    a.setWindowIcon(QIcon(":/images/SciCat_logo_icon.svg"));
#endif
    Dialog w;
    Login l;
    l.connectToGui(&w);
    l.show();

    return a.exec();
}
