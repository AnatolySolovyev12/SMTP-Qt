
#include "smtp.h"

Smtp::Smtp(const QString& user, const QString& pass, const QString& host, int port, int timeout)
{
    //Проверяем версию SSL

    qDebug() << QSslSocket::supportsSsl() << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::sslLibraryVersionString() << QSslSocket::sslLibraryVersionNumber() << "\n";
   
    socket = new QSslSocket(this);

    //сигналы для них используются бибилиотечные
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyReadFromSocket()));
    connect(socket, SIGNAL(connected()), this, SLOT(connectedInfo()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceivedInfo(QAbstractSocket::SocketError))); // параметры прописывать необязательно но можно.
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChangedInfo(QAbstractSocket::SocketState))); // параметры прописывать необязательно но можно.
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectedInfo()));

    this->user = user;
    this->pass = pass;
    this->host = host;
    this->port = port;
    this->timeout = timeout;
}



void Smtp::sendMail(const QString& from, const QString& to, const QString& subject, const QString& body)
{
    message = "To: " + to + "\n"; // наполняем сообщение
    message.append("From: " + from + "\n");
    message.append("Subject: " + subject + "\n");
    message.append(body);
    message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n")); // зачем то производится замена символа \n на \r\n
    message.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n")); // аналогично. 
    this->from = from; //почему то без укзаания класса не присваивается. Можно "Smtp::"
    rcpt = to;
    state = Init;
    socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS

    if (!socket->waitForConnected(timeout)) // ожидаем подключения сокета по таймауту.
    {
        qDebug() << socket->errorString();
    }

    t = new QTextStream(socket); // будем принимать поток данных с указанного сокета
}


Smtp::~Smtp()
{
    delete t;
    delete socket;

    qDebug() << "\nObject, socket and t was delete.";
}


void Smtp::stateChangedInfo(QAbstractSocket::SocketState socketState) // Это сигнал который генерируетсмя всякий раз когда меняется состояние сокета. Идёт в комплекте с библиотекой. socketState это новое состояние. 
{
    qDebug() << "\nstateChanged " << socketState;
}


void Smtp::errorReceivedInfo(QAbstractSocket::SocketError socketError) // Сигнал который выдаётся после возгникновения ошибки. Идёт в комплекте с библиотекой. socketError описывает тип произошедшей ошибки.
{
    qDebug() << "\nerror " << socketError;
}


void Smtp::disconnectedInfo()
{
    qDebug() << "\ndisconneted";
    qDebug() << "\nerror " << socket->errorString();
}


void Smtp::connectedInfo()
{
    qDebug() << "\nConnected";
}


//Важно. Для того чтобы пройти авторизацию в Gmail, необходимо создать 16-значный пароль приложения. Для этого необходимо ообязательно включить 2-х факторную авторизацию иначе не пустит.
void Smtp::readyReadFromSocket()
{
    userByte = user.toUtf8();
    passByte = pass.toUtf8();

    qDebug() << "\nreadyReadFromSocket()\n";

    // SMTP is line-oriented
    
    //Будем принимать ответы от сокета в виде 3-х значных кодов с пояснением.

    int count = 1;

    QString responseLine;

    do
    {
        responseLine = socket->readLine(); // к данным в конце всегда добавляется завершающий символ "\0"
        response += responseLine;

        qDebug() << count << " - " << responseLine;

        count++;

    } while (socket->canReadLine() && responseLine[3] != ' ');

    responseLine.truncate(3); // отсекаем строку по индексу позиции. В частности отсекаем "\0"

    //После подключения к порту сервера он должен ответить с кодом 220

    if (state == Init && responseLine == "220")
    {
        // banner was okay, let's go on
        *t << "EHLO localhost" << "\r\n"; // начинаем сессию с помощью команды EHLO по стандарту для ESMTP протокола. 
        //Сервер ответит успешно (код 250), отказом (код 550) или ошибкой (код 500, 501, 502, 504 или 421), в зависимости от его конфигурации. 
        // Сервер ESMTP возвращает код 250 OK в многострочном ответе со своим доменом и списком ключевых слов для обозначения поддерживаемых расширений. 
       
        t->flush(); // сбрасываем все буфферизированные данные, ожидающие записи на устройство.

        state = HandShake;
    }

    //После полслдания ему EHLO и домена он должен ответить с кодом 250

    else if (state == HandShake && responseLine == "250")
    {
        if (!socket->isEncrypted()) // проверяем зашифровано ли подключение
        {
            socket->startClientEncryption(); // запускаем отложенное подтверждение SSL для клиентского подключения. Если уже шифруемся то ничего не произойдёт или выдаст некритичное уведомление об ошибке.
            if (!socket->waitForEncrypted(timeout))
            {
                qDebug() << "IF NOT startClientEncryption()";
                qDebug() << socket->errorString();
                state = Close;
            }
        }

        //Send EHLO once again but now encrypted // Так то в самом наче делали зашифрованное подключение

        *t << "EHLO localhost" << "\r\n"; // непонятно зачем повторный запрос делается.
        t->flush();
        state = Auth;
    }

    else if (state == Auth && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "\nAuth";
        *t << "AUTH LOGIN" << "\r\n"; // посылаем команду на авторизацию
        t->flush();
        state = User;
    }

    // если команда корректная то сервер пришлёт 334

    else if (state == User && responseLine == "334")
    {
        //Trying User        
        qDebug() << "\nUsername";
        //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
        //https://developers.google.com/gmail/xoauth2_protocol
        *t << QByteArray().append(userByte).toBase64() << "\r\n";
        t->flush();

        state = Pass;
    }

   //После отправки логина должны получить 334

    else if (state == Pass && responseLine == "334")
    {
        //Trying pass
        qDebug() << "\nPass";
        *t << QByteArray().append(passByte).toBase64() << "\r\n";
        t->flush();

        state = Mail;
    }

    //После отправки пароля должны получить 235 если все данные корректные или 535 если авторизация не пройдена

    else if (state == Mail && responseLine == "235")
    {
        // HELO response was okay (well, it has to be)

        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        qDebug() << "\nMAIL FROM:<" << from << ">";
        *t << "MAIL FROM:<" << from << ">\r\n";
        t->flush();
        state = Rcpt;
    }

    //Если почта авторизовались и правильно выбрадли от кого то придёт 250 

    else if (state == Rcpt && responseLine == "250")
    {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *t << "RCPT TO:<" << rcpt << ">\r\n"; // можно несколько раз отправить команду и получиться несколько адресатов
        t->flush();
        state = Data;
    }

    //Кому отправляем тоже должно быть корректно передано и получим код 250

    else if (state == Data && responseLine == "250")
    {

        *t << "DATA\r\n";
        t->flush();
        state = Body;
    }

    //После послания DATA можно формировать тело письма если получен код 354

    else if (state == Body && responseLine == "354")
    {

        *t << message << "\r\n.\r\n";
        t->flush();
        state = Quit;
    }

    // Если сообщение принято к доставке то придёт 250

    else if (state == Quit && responseLine == "250")
    {
        QMessageBox::information(0, tr("Qt Simple SMTP client"), tr("Message was send.\n\n"));
        *t << "QUIT\r\n";
        t->flush();
        state = Close;
        emit status(tr("Message sent")); // emit - макрос сомнительной полезнлости
    }


    else if (state == Close)
    {
        deleteLater(); // отложенное удаление объекта после возврата в цикл обработчика событий.
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
