#include "SMTPQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SMTPQt w;
    w.show();
    return a.exec();
}
