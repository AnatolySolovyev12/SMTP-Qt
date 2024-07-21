#include "SMTPQt.h"
#include "Smtp.h"

#include <QtNetwork/QSslSocket>

SMTPQt::SMTPQt(QWidget *parent)
    : QMainWindow(parent)
{
    SMTPQt::ui.setupUi(this);
    connect(ui.sendBtn_4, SIGNAL(clicked()), this, SLOT(sendMailfromButton()));
    connect(ui.exitBtn_4, SIGNAL(clicked()), this, SLOT(close()));
}

void SMTPQt::sendMailfromButton()
{
  
    Smtp* smtp = new Smtp(ui.uname->text(), ui.paswd->text(), ui.server->text(), ui.port->text().toInt());

    connect(smtp, SIGNAL(status(QString)), this, SLOT(MessegeAboutMailSend(QString)));

    smtp->sendMail(ui.uname->text(), ui.rcpt->text(), ui.subject->text(), ui.msg->toPlainText());
    
}

void SMTPQt::MessegeAboutMailSend(QString status)
{
    if (status == "Message sent")
        QMessageBox::warning(0, tr("Qt Simple SMTP client"), tr("Message sent!\n\n"));
}

SMTPQt::~SMTPQt()
{}