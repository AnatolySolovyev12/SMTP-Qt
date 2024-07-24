
#ifndef SMTP_H
#define SMTP_H


#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QtWidgets/QMessageBox>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>



class Smtp : public QObject
{
    Q_OBJECT


public:
    Smtp(const QString& user, const QString& pass,
        const QString& host, int port = 465, int timeout = 30000);
    ~Smtp();

    void sendMail(const QString& from, const QString& to,
        const QString& subject, const QString& body, QStringList files = QStringList()); // QStringList files = QStringList() - шаблонизирует функцию. “ак сможем выборочно добавл€ть или нет последний параметр.


signals:
    void status(const QString&); // самоделашный сигнал дл€ вывод в объект основного класса. 

private slots:
    //сигналы дл€ них используютс€ бибилиотечные
    void stateChangedInfo(QAbstractSocket::SocketState socketState);
    void errorReceivedInfo(QAbstractSocket::SocketError socketError);
    void disconnectedInfo();
    void connectedInfo();
    void readyReadFromSocket();


private:
    int timeout;
    QString message;
    QTextStream* t = nullptr;
    QSslSocket* socket = nullptr;
    QString from;
    QString rcpt;
    QString response;
    QString user;
    QString pass;
    QByteArray userByte;
    QByteArray passByte;
    QString host;
    int port;
    enum states { Tls, HandShake, Auth, User, Pass, Rcpt, Mail, Data, Init, Body, Quit, Close };
    int state = 8;

};
#endif
