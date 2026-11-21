#pragma once

#include "appsettings.h"

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// The preferences as bindable properties. Every change is stored at once.
class Settings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use Backend.settings")

    Q_PROPERTY(bool usePgroupAsDefaultPath READ usePgroupAsDefaultPath WRITE setUsePgroupAsDefaultPath NOTIFY changed)
    Q_PROPERTY(bool guessDatasetName READ guessDatasetName WRITE setGuessDatasetName NOTIFY changed)
    Q_PROPERTY(bool investigatorIsPI READ investigatorIsPI WRITE setInvestigatorIsPI NOTIFY changed)
    Q_PROPERTY(bool saveProposalInfo READ saveProposalInfo WRITE setSaveProposalInfo NOTIFY changed)
    Q_PROPERTY(bool loadProposalInfo READ loadProposalInfo WRITE setLoadProposalInfo NOTIFY changed)
    Q_PROPERTY(bool enableIngest READ enableIngest WRITE setEnableIngest NOTIFY changed)
    Q_PROPERTY(bool enableArchive READ enableArchive WRITE setEnableArchive NOTIFY changed)
    Q_PROPERTY(bool requestAlreadyRetrieved READ requestAlreadyRetrieved WRITE setRequestAlreadyRetrieved NOTIFY changed)
    Q_PROPERTY(bool saveLog READ saveLog WRITE setSaveLog NOTIFY changed)
    Q_PROPERTY(bool deleteLog READ deleteLog WRITE setDeleteLog NOTIFY changed)
    Q_PROPERTY(bool showCommand READ showCommand WRITE setShowCommand NOTIFY changed)
    Q_PROPERTY(bool disableSystemCalls READ disableSystemCalls WRITE setDisableSystemCalls NOTIFY changed)
    Q_PROPERTY(QString logPath READ logPath WRITE setLogPath NOTIFY changed)
    Q_PROPERTY(QString metadataFileName READ metadataFileName WRITE setMetadataFileName NOTIFY changed)
    Q_PROPERTY(QString fileListingFileName READ fileListingFileName WRITE setFileListingFileName NOTIFY changed)
    Q_PROPERTY(QString cliPath READ cliPath WRITE setCliPath NOTIFY changed)
    Q_PROPERTY(bool cliFound READ cliFound NOTIFY changed)
    Q_PROPERTY(QString location READ location CONSTANT)
    Q_PROPERTY(bool canUndoReset READ canUndoReset NOTIFY canUndoResetChanged)

public:
    explicit Settings(scicat::AppSettings *store, QObject *parent = nullptr);

    scicat::AppSettings *store() const { return m_store; }

    bool usePgroupAsDefaultPath() const { return flag("usePgroupAsDefaultPath"); }
    void setUsePgroupAsDefaultPath(bool on) { set("usePgroupAsDefaultPath", on); }
    bool guessDatasetName() const { return flag("guessDatasetname"); }
    void setGuessDatasetName(bool on) { set("guessDatasetname", on); }
    bool investigatorIsPI() const { return flag("investigatorIsPI"); }
    void setInvestigatorIsPI(bool on) { set("investigatorIsPI", on); }
    bool saveProposalInfo() const { return flag("saveProposalInfo"); }
    void setSaveProposalInfo(bool on) { set("saveProposalInfo", on); }
    bool loadProposalInfo() const { return flag("loadProposalInfo"); }
    void setLoadProposalInfo(bool on) { set("loadProposalInfo", on); }
    bool enableIngest() const { return flag("enableIngest"); }
    void setEnableIngest(bool on) { set("enableIngest", on); }
    bool enableArchive() const { return flag("enableArchive"); }
    void setEnableArchive(bool on) { set("enableArchive", on); }
    bool requestAlreadyRetrieved() const { return flag("requestAlreadyRetrievedData"); }
    void setRequestAlreadyRetrieved(bool on) { set("requestAlreadyRetrievedData", on); }
    bool saveLog() const { return flag("saveLog"); }
    void setSaveLog(bool on) { set("saveLog", on); }
    bool deleteLog() const { return flag("deleteLog"); }
    void setDeleteLog(bool on) { set("deleteLog", on); }
    bool showCommand() const { return flag("printIngestion"); }
    void setShowCommand(bool on) { set("printIngestion", on); }
    bool disableSystemCalls() const { return flag("disableSystemCalls"); }
    void setDisableSystemCalls(bool on) { set("disableSystemCalls", on); }
    QString logPath() const { return text("logPath"); }
    void setLogPath(const QString &path);
    QString metadataFileName() const { return text("filenameMetadata"); }
    void setMetadataFileName(const QString &name);
    QString fileListingFileName() const { return text("filenameFilelisting"); }
    void setFileListingFileName(const QString &name);
    QString cliPath() const { return text("scicatCliBinary"); }
    void setCliPath(const QString &path) { set("scicatCliBinary", path); }
    bool cliFound() const;
    QString location() const { return m_store->location(); }
    bool canUndoReset() const { return !m_beforeReset.isEmpty(); }

    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void undoReset();
    Q_INVOKABLE void forgetReset();
    Q_INVOKABLE QString toLocalPath(const QUrl &url) const { return url.toLocalFile(); }

    // interface state
    Q_INVOKABLE QVariant ui(const QString &key, const QVariant &fallback = QVariant()) const { return m_store->uiValue(key, fallback); }
    Q_INVOKABLE void setUi(const QString &key, const QVariant &value) { m_store->setUiValue(key, value); }

signals:
    void changed();
    void canUndoResetChanged();

private:
    bool flag(const char *key) const { return m_store->boolean(QLatin1String(key)); }
    QString text(const char *key) const { return m_store->string(QLatin1String(key)); }
    void set(const char *key, const QVariant &value);

    scicat::AppSettings *m_store;
    QVariantMap m_beforeReset;
};
