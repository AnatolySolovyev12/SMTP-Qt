#include "SMTPQt.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SMTPQt w;
    w.setWindowIcon(QIcon("icon.png"));
    w.setWindowTitle("SMTP client by Solovev");
    w.show();
    return a.exec();
}
