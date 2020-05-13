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
    void authenticateUser(const QString pass);
    QProcess *kerberosAuth;
    void openGate();
    void errorMessage(QString msg);

private slots:
    void getLoginData();
    void authenticationFailed(QProcess::ProcessError error);
    void authenticationFinished(int val);
    void readKeberosOutput();



signals:


};

#endif // LOGIN_H
