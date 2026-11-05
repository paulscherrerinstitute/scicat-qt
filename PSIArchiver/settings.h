#ifndef SETTINGS_H
#define SETTINGS_H
#include "dialog.h"
#include "ui_settings.h"


class Settings : public QDialog
{
    Q_OBJECT

public:
    enum class Context { BeforeLogin, AfterLogin };
    Settings(Dialog *base, Context context, QWidget *parent = nullptr);
    ~Settings();
    void updateSettings();

public slots:
    void toggleAdvancedSettings(int val);
    void updateServerURL(int val);
    void logFileBrowser();
    void reset();
    void acceptChanges();
    void checkFilelistingFile();
    void checkMetadataFile();

private:
    Ui::Settings *ui;
    Dialog *baseUi;
    Context context;
};

#endif // SETTINGS_H
