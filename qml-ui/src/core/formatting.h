#pragma once

#include <QDateTime>
#include <QString>

namespace scicat {

QString formatBytes(qint64 bytes);            // "4.0 GiB"; empty for unknown (negative) sizes
QString formatCount(qint64 count);            // "150,000" in the reader's locale
QString formatDateTime(const QDateTime &time); // short local date and time
QString formatDuration(qint64 seconds);       // "42 s", "3 min 5 s", "2 h 10 min"

} // namespace scicat
