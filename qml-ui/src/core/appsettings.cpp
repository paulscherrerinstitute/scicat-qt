#include "appsettings.h"

#include <QDir>
#include <QVersionNumber>

namespace scicat {

namespace {
const QVersionNumber settingsVersion(1, 0);
}

AppSettings::AppSettings(const QString &iniFile)
{
    if (iniFile.isEmpty())
        m_store = std::make_unique<QSettings>(QStringLiteral("PSI"), QStringLiteral("SciCatQml"));
    else
        m_store = std::make_unique<QSettings>(iniFile, QSettings::IniFormat);

    // start clean when the major version of the stored settings differs
    const QVersionNumber stored = QVersionNumber::fromString(m_store->value(QStringLiteral("version")).toString());
    if (stored.isNull() || stored.majorVersion() != settingsVersion.majorVersion()) {
        m_store->clear();
        m_store->setValue(QStringLiteral("version"), settingsVersion.toString());
    }
}

QVariantMap AppSettings::defaults()
{
    return {
        {QStringLiteral("scicatCliBinary"), QStringLiteral("scicat-cli")},
        {QStringLiteral("logPath"), QDir::homePath() + QStringLiteral("/.scicat/log")},
        {QStringLiteral("filenameFilelisting"), QStringLiteral("filelisting.txt")},
        {QStringLiteral("filenameMetadata"), QStringLiteral("metadata.json")},
        {QStringLiteral("deleteLog"), false},
        {QStringLiteral("serverID"), 0},
        {QStringLiteral("printIngestion"), true},
        {QStringLiteral("disableSystemCalls"), false},
        {QStringLiteral("usePgroupAsDefaultPath"), true},
        {QStringLiteral("saveProposalInfo"), true},
        {QStringLiteral("loadProposalInfo"), true},
        {QStringLiteral("saveLog"), true},
        {QStringLiteral("enableArchive"), true},
        {QStringLiteral("enableIngest"), true},
        {QStringLiteral("investigatorIsPI"), false},
        {QStringLiteral("requestAlreadyRetrievedData"), true},
        {QStringLiteral("guessDatasetname"), true},
    };
}

QVariant AppSettings::value(const QString &key) const
{
    const QVariant fallback = defaults().value(key);
    QVariant stored = m_store->value(key, fallback);
    // ini files hand back strings: give the value the type of its default
    if (fallback.isValid() && stored.metaType() != fallback.metaType())
        stored.convert(fallback.metaType());
    return stored;
}

void AppSettings::setValue(const QString &key, const QVariant &value)
{
    m_store->setValue(key, value);
}

QVariantMap AppSettings::snapshot() const
{
    QVariantMap values;
    const QVariantMap all = defaults();
    for (auto it = all.constBegin(); it != all.constEnd(); ++it)
        values.insert(it.key(), value(it.key()));
    return values;
}

void AppSettings::restore(const QVariantMap &values)
{
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        m_store->setValue(it.key(), it.value());
}

// The preferences, as the Settings page shows them. Which server is used is chosen on the sign-in page, and where
// scicat-cli is was found by looking for it: a reset must not quietly change either.
void AppSettings::resetToDefaults()
{
    QVariantMap values = defaults();
    values.remove(QStringLiteral("serverID"));
    values.remove(QStringLiteral("scicatCliBinary"));
    restore(values);
}

QVariant AppSettings::uiValue(const QString &key, const QVariant &fallback) const
{
    return m_store->value(QStringLiteral("ui/") + key, fallback);
}

void AppSettings::setUiValue(const QString &key, const QVariant &value)
{
    m_store->setValue(QStringLiteral("ui/") + key, value);
}

QString AppSettings::location() const
{
    return m_store->fileName();
}

QStringList AppSettings::keys() const
{
    return m_store->allKeys();
}

QString AppSettings::fixedFileName(const QString &name, const QString &suffix, const QString &fallback)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return fallback;
    return trimmed.endsWith(suffix, Qt::CaseInsensitive) ? trimmed : trimmed + suffix;
}

} // namespace scicat
