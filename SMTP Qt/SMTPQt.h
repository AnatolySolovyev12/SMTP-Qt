#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SMTPQt.h"
#include <QMessageBox>

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

private:
    Ui::SMTPQtClass ui;
};
