#include "filescan.h"

#include <QDirIterator>
#include <QFileInfo>

namespace scicat {

ScanResult scanPath(const QString &path, const std::atomic_bool *cancel)
{
    ScanResult result;
    const QFileInfo info(path);
    result.exists = info.exists();
    result.isFolder = info.isDir();
    if (!result.exists)
        return result;
    if (!result.isFolder) {
        result.files = 1;
        result.bytes = info.size();
        return result;
    }
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (cancel && cancel->load(std::memory_order_relaxed)) {
            result.cancelled = true;
            return result;
        }
        const QFileInfo file = it.nextFileInfo();
        ++result.files;
        result.bytes += file.size();
    }
    return result;
}

} // namespace scicat
