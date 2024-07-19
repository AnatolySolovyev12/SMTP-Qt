
#ifndef SMTP_H
#define SMTP_H


#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QtWidgets/QMessageBox>
#include <QByteArray>



class Smtp : public QObject
{
    Q_OBJECT


public:
    Smtp(const QString& user, const QString& pass,
        const QString& host, int port = 465, int timeout = 30000);
    ~Smtp();

    void sendMail(const QString& from, const QString& to,
        const QString& subject, const QString& body);

signals:
    void status(const QString&);

private slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void errorReceived(QAbstractSocket::SocketError socketError);
    void disconnected();
    void connected();
    void readyRead();

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
