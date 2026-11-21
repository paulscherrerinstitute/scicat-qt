#include "formatting.h"

#include <QLocale>

namespace scicat {

QString formatBytes(qint64 bytes)
{
    if (bytes < 0)
        return QString();
    return QLocale().formattedDataSize(bytes, bytes < 1024 ? 0 : 1, QLocale::DataSizeIecFormat);
}

QString formatCount(qint64 count)
{
    return QLocale().toString(count);
}

QString formatDateTime(const QDateTime &time)
{
    if (!time.isValid())
        return QString();
    return QLocale().toString(time.toLocalTime(), QLocale::ShortFormat);
}

QString formatDuration(qint64 seconds)
{
    if (seconds < 60)
        return QStringLiteral("%1 s").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1 min %2 s").arg(seconds / 60).arg(seconds % 60);
    return QStringLiteral("%1 h %2 min").arg(seconds / 3600).arg((seconds % 3600) / 60);
}

} // namespace scicat
