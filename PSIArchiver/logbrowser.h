#ifndef LOGBROWSER_H
#define LOGBROWSER_H

#include "dialog.h"
#include "ui_logbrowser.h"


class LogBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit LogBrowser(QWidget *parent = nullptr);
    void clear();
    ~LogBrowser();
    Ui::LogBrowser *ui;

private slots:
    void saveLog();

};

#endif // LOGBROWSER_H
