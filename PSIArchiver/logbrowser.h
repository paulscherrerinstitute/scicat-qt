#ifndef LOGBROWSER_H
#define LOGBROWSER_H

#include <QDialog>

namespace Ui {
class LogBrowser;
}

class LogBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit LogBrowser(QWidget *parent = nullptr);
    ~LogBrowser();

private:
    Ui::LogBrowser *ui;
};

#endif // LOGBROWSER_H
