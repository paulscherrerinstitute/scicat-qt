#include "messagehandlergui.h"
#include <QDebug>
#include <memory>

MessageHandlerGUI::MessageHandlerGUI(QLabel* lbl, QString msg):m_lbl(lbl), m_msg(msg){

}


void MessageHandlerGUI::show(int time){
    m_time = time;
    m_lbl->setText(m_msg);
    m_lbl->setVisible(true);
    QTimer::singleShot(time, m_lbl, &QLabel::hide);
}

void MessageHandlerGUI::show(){
    m_lbl->setText(m_msg);
    m_lbl->setVisible(true);
}

void MessageHandlerGUI::hide(){
    m_lbl->setVisible(false);
}

MessageHandlerGUI::~MessageHandlerGUI(){
    if (m_time < 0){
        MessageHandlerGUI::hide();
    }
}

std::shared_ptr<MessageHandlerGUI> MessageHandlerGUI::sendWarning(QLabel* lbl, QString msg, int time){
    std::shared_ptr<MessageHandlerGUI> msgHandler = std::make_shared<MessageHandlerGUI>(lbl, msg);
    lbl->setStyleSheet("QLabel{color:rgb(218, 0, 0)}");
    if (time>0){
        msgHandler->show(time);
    } else {
        msgHandler->show();
    }
    return msgHandler;
}

std::shared_ptr<MessageHandlerGUI> MessageHandlerGUI::sendMessage(QLabel* lbl, QString msg, int time){
    std::shared_ptr<MessageHandlerGUI> msgHandler = std::make_shared<MessageHandlerGUI>(lbl, msg);
    // no fixed colour, so the native text colour applies in light and dark mode
    lbl->setStyleSheet("");
    if (time>0){
        msgHandler->show(time);
    } else {
        msgHandler->show();
    }
    return msgHandler;
}
