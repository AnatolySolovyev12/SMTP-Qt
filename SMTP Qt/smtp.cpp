
#include "smtp.h"

Smtp::Smtp(const QString& user, const QString& pass, const QString& host, int port, int timeout)
{
    socket = new QSslSocket(this);

    //������� ��� ��� ������������ �������������
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyReadFromSocket()));
    connect(socket, SIGNAL(connected()), this, SLOT(connectedInfo()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceivedInfo(QAbstractSocket::SocketError))); // ��������� ����������� ������������� �� �����.
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChangedInfo(QAbstractSocket::SocketState))); // ��������� ����������� ������������� �� �����.
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectedInfo()));

    this->user = user;
    this->pass = pass;
    this->host = host;
    this->port = port;
    this->timeout = timeout;
}



void Smtp::sendMail(const QString& from, const QString& to, const QString& subject, const QString& body)
{
    message = "To: " + to + "\n"; // ��������� ���������
    message.append("From: " + from + "\n");
    message.append("Subject: " + subject + "\n");
    message.append(body);
    message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n")); // ����� �� ������������ ������ ������� \n �� \r\n
    message.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n")); // ����������. 
    this->from = from; //������ �� ��� �������� ������ �� �������������. ����� "Smtp::"
    rcpt = to;
    state = Init;
    socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS

    if (!socket->waitForConnected(timeout)) // ������� ����������� ������ �� ��������.
    {
        qDebug() << socket->errorString();
    }

    t = new QTextStream(socket); // ����� ��������� ����� ������ � ���������� ������
}




Smtp::~Smtp()
{
    delete t;
    delete socket;
}




void Smtp::stateChangedInfo(QAbstractSocket::SocketState socketState) // ��� ������ ������� ������������� ������ ��� ����� �������� ��������� ������. ��� � ��������� � �����������. socketState ��� ����� ���������. 
{
    qDebug() << "stateChanged " << socketState;
}



void Smtp::errorReceivedInfo(QAbstractSocket::SocketError socketError) // ������ ������� ������� ����� �������������� ������. ��� � ��������� � �����������. socketError ��������� ��� ������������ ������.
{
    qDebug() << "error " << socketError;
}



void Smtp::disconnectedInfo()
{

    qDebug() << "disconneted";
    qDebug() << "error " << socket->errorString();
}



void Smtp::connectedInfo()
{
    qDebug() << "\nConnected\n";
}



//�����. ��� ���� ����� ������ ����������� � Gmail, ���������� ������� 16-������� ������ ����������. ��� ����� ���������� ������������ �������� 2-� ��������� ����������� ����� �� ������.

void Smtp::readyReadFromSocket()
{
    userByte = user.toUtf8();
    passByte = pass.toUtf8();

    qDebug() << "\nreadyRead\n";

    // SMTP is line-oriented
    // 
    //����� ��������� ������ �� ������ � ���� 3-� ������� �����.
    int count = 1;
    QString responseLine;

    do
    {
        responseLine = socket->readLine(); // � ������ � ����� ������ ����������� ����������� ������ "\0"
        response += responseLine;

        qDebug() << count << " - " << responseLine;
        if (!socket->canReadLine())
        {
            qDebug() << count << " - " << response;
        }
        count++;

    } while (socket->canReadLine() && responseLine[3] != ' ');

    responseLine.truncate(3); // �������� ������ �� ������� �������. � ��������� �������� "\0"

  //  qDebug() << "Server response code:" << responseLine;
  // qDebug() << "Server response: " << response;

    //����� ����������� � ����� ������� �� ������ �������� � ����� 220

    if (state == Init && responseLine == "220")
    {
        // banner was okay, let's go on
        *t << "EHLO localhost" << "\r\n"; // �������� ������ � ������� ������� EHLO �� ��������� ��� ESMTP ���������. 
        //������ ������� ������� (��� 250), ������� (��� 550) ��� ������� (��� 500, 501, 502, 504 ��� 421), � ����������� �� ��� ������������. 
        // ������ ESMTP ���������� ��� 250 OK � ������������� ������ �� ����� ������� � ������� �������� ���� ��� ����������� �������������� ����������. 
       
        t->flush(); // ���������� ��� ����������������� ������, ��������� ������ �� ����������.

        state = HandShake;
    }

    //����� ���������� ��� EHLO � ������ �� ������ �������� � ����� 250

    else if (state == HandShake && responseLine == "250")
    {
        if (!socket->isEncrypted()) // ��������� ����������� �� �����������
        {
            socket->startClientEncryption(); // ��������� ���������� ������������� SSL ��� ����������� �����������. ���� ��� ��������� �� ������ �� ��������� ��� ������ ����������� ����������� �� ������.
            if (!socket->waitForEncrypted(timeout))
            {
                qDebug() << "IF NOT startClientEncryption()";
                qDebug() << socket->errorString();
                state = Close;
            }
        }

        //Send EHLO once again but now encrypted // ��� �� � ����� ���� ������ ������������� �����������

        *t << "EHLO localhost" << "\r\n"; // ��������� ����� ��������� ������ ��������.
        t->flush();
        state = Auth;
    }

    else if (state == Auth && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "Auth";
        *t << "AUTH LOGIN" << "\r\n"; // �������� ������� �� �����������
        t->flush();
        state = User;
    }

    // ���� ������� ���������� �� ������ ������ 334

    else if (state == User && responseLine == "334")
    {
        //Trying User        
        qDebug() << "Username";
        //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
        //https://developers.google.com/gmail/xoauth2_protocol
        *t << QByteArray().append(userByte).toBase64() << "\r\n";
        t->flush();

        state = Pass;
    }

   //����� �������� ������ ������ �������� 334

    else if (state == Pass && responseLine == "334")
    {
        //Trying pass
        qDebug() << "Pass";
        *t << QByteArray().append(passByte).toBase64() << "\r\n";
        t->flush();

        state = Mail;
    }

    //����� �������� ������ ������ �������� 235 ���� ��� ������ ���������� ��� 535 ���� ����������� �� ��������

    else if (state == Mail && responseLine == "235")
    {
        // HELO response was okay (well, it has to be)

        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        qDebug() << "MAIL FROM:<" << from << ">";
        *t << "MAIL FROM:<" << from << ">\r\n";
        t->flush();
        state = Rcpt;
    }

    //���� ����� �������������� � ��������� �������� �� ���� �� ����� 250 

    else if (state == Rcpt && responseLine == "250")
    {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *t << "RCPT TO:<" << rcpt << ">\r\n"; // ����� ��������� ��� ��������� ������� � ���������� ��������� ���������
        t->flush();
        state = Data;
    }

    //���� ���������� ���� ������ ���� ��������� �������� � ������� ��� 250

    else if (state == Data && responseLine == "250")
    {

        *t << "DATA\r\n";
        t->flush();
        state = Body;
    }

    //����� �������� DATA ����� ����������� ���� ������ ���� ������� ��� 354

    else if (state == Body && responseLine == "354")
    {

        *t << message << "\r\n.\r\n";
        t->flush();
        state = Quit;
    }

    // ���� ��������� ������� � �������� �� ����� 250

    else if (state == Quit && responseLine == "250")
    {

        *t << "QUIT\r\n";
        t->flush();
        // here, we just close.
        state = Close;
        emit status(tr("Message sent")); // emit - ������ ������������ �����������

       // return;
    }


    else if (state == Close)
    {
        deleteLater(); // ���������� �������� ������� ����� �������� � ���� ����������� �������.
        return;
    }
    

    else
    {
        // something broke.
        QMessageBox::warning(0, tr("Qt Simple SMTP client"), tr("Unexpected reply from SMTP server:\n\n") + response);
        state = Close;
        emit status(tr("Failed to send message"));
    }
    response = "";
}
