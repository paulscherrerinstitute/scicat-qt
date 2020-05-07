#ifndef LOGIN_H
#define LOGIN_H

#include "dialog.h"
#include "ui_login.h"


class Login : public QDialog
{
    Q_OBJECT
public:
    Login(QWidget *parent = nullptr);
    ~Login();
    bool accessGranted = false;
    void connectToGui(Dialog *ui);
private:
    Ui::Login *uiLogin;
    Dialog *baseUi;
private slots:
    void getLoginData();


signals:


};

#endif // LOGIN_H
