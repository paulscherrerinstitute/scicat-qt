#ifndef SETTINGS_H
#define SETTINGS_H
#include "dialog.h"
#include "ui_settings.h"


class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();
    void connectToUi(Dialog *uiDialog);
    void updateSettings();

public slots:
    void toggleAdvancedSettings(int val);
    void updateServerURL(int val);
    void logFileBrowser();
    void reset();
    void acceptChanges();

private:
    Ui::Settings *ui;
    Dialog *baseUi;
};

#endif // SETTINGS_H
