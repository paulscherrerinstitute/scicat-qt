#include "archiverutils.h"
#include "dialog.h"
#include "login.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    Login l;
    l.connectToGui(&w);
    l.show();

    return a.exec();
}
