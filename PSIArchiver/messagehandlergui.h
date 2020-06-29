#ifndef MESSAGEHANDLERGUI_H
#define MESSAGEHANDLERGUI_H


#include <QTimer>
#include <QLabel>

class MessageHandlerGUI
{
public:
    MessageHandlerGUI(QLabel* lbl, QString msg);
    ~MessageHandlerGUI();
    void show();
    void show(int time);
    void hide();
    static std::shared_ptr<MessageHandlerGUI> sendWarning(QLabel *lbl, QString msg, int time=5000);
    static std::shared_ptr<MessageHandlerGUI> sendMessage(QLabel *lbl, QString msg, int time=-1);

private:
    QLabel* m_lbl;
    QString m_msg;
    int m_time=-1;

};

#endif // MESSAGEHANDLERGUI_H
