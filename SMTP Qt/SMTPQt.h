#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SMTPQt.h"
#include <QMessageBox>
#include "Smtp.h"
#include <QFileDialog>

namespace Ui {
    class SMTPQt;
}

class SMTPQt : public QMainWindow
{
    Q_OBJECT

public:
    SMTPQt(QWidget *parent = nullptr);
    ~SMTPQt();


private slots:
    void sendMailfromButton();
    void MessegeAboutMailSend(QString);
    void browse();

private:
    Ui::SMTPQtClass ui;
    Smtp* smtp = nullptr;
    QStringList files;

};
