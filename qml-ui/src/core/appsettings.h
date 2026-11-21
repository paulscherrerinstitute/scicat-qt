#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <memory>

namespace scicat {

// Persistent preferences with the defaults of the Widgets GUI (defaultSettings_t in PSIArchiver/dialog.h).
// The keys are the same; the store is a different one, so the two applications never overwrite each other.
class AppSettings
{
public:
    static constexpr int maxFilesPerDataset = 150000;

    // iniFile empty: the platform's native store for PSI/SciCatQml
    explicit AppSettings(const QString &iniFile = QString());

    static QVariantMap defaults();
    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &value);
    bool boolean(const QString &key) const { return value(key).toBool(); }
    QString string(const QString &key) const { return value(key).toString(); }

    // the documented preferences only, not window geometry and other interface state
    QVariantMap snapshot() const;
    void restore(const QVariantMap &values);
    void resetToDefaults();

    // interface state (window size, last pgroup, dismissed hints) lives under "ui/"
    QVariant uiValue(const QString &key, const QVariant &fallback = QVariant()) const;
    void setUiValue(const QString &key, const QVariant &value);

    QString location() const;
    // everything that is stored, interface state included
    QStringList keys() const;

    // "name" -> "name.json" / default when empty, as the Widgets settings dialog does on leaving the field
    static QString fixedFileName(const QString &name, const QString &suffix, const QString &fallback);

private:
    std::unique_ptr<QSettings> m_store;
};

} // namespace scicat
