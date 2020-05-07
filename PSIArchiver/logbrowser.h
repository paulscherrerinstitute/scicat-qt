#ifndef LOGBROWSER_H
#define LOGBROWSER_H

#include "ui_logbrowser.h"


class LogBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit LogBrowser(QWidget *parent = nullptr);
    ~LogBrowser();
    Ui::LogBrowser *ui;

};

#endif // LOGBROWSER_H
