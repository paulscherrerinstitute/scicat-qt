#include "settings.h"

#include <QFileInfo>
#include <QStandardPaths>

Settings::Settings(scicat::AppSettings *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
}

void Settings::set(const char *key, const QVariant &value)
{
    if (m_store->value(QLatin1String(key)) == value)
        return;
    m_store->setValue(QLatin1String(key), value);
    // "Undo" of a reset brings back everything as it was before; after a change made since, that would undo too much
    forgetReset();
    emit changed();
}

void Settings::setLogPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    set("logPath", trimmed.isEmpty() ? scicat::AppSettings::defaults().value(QStringLiteral("logPath")) : QVariant(trimmed));
    emit changed(); // also when an empty entry falls back to the stored default, so the field shows it
}

void Settings::setMetadataFileName(const QString &name)
{
    set("filenameMetadata", scicat::AppSettings::fixedFileName(name, QStringLiteral(".json"), QStringLiteral("metadata.json")));
    emit changed();
}

void Settings::setFileListingFileName(const QString &name)
{
    set("filenameFilelisting", scicat::AppSettings::fixedFileName(name, QStringLiteral(".txt"), QStringLiteral("filelisting.txt")));
    emit changed();
}

bool Settings::cliFound() const
{
    const QString path = cliPath();
    const QFileInfo info(path);
    if (info.isAbsolute())
        return info.isExecutable();
    return !QStandardPaths::findExecutable(path).isEmpty();
}

void Settings::resetToDefaults()
{
    m_beforeReset = m_store->snapshot();
    m_store->resetToDefaults();
    emit changed();
    emit canUndoResetChanged();
}

void Settings::undoReset()
{
    if (m_beforeReset.isEmpty())
        return;
    m_store->restore(m_beforeReset);
    forgetReset();
    emit changed();
}

void Settings::forgetReset()
{
    if (m_beforeReset.isEmpty())
        return;
    m_beforeReset.clear();
    emit canUndoResetChanged();
}
