#pragma once

#include <QString>
#include <atomic>

namespace scicat {

struct ScanResult
{
    qint64 files = 0;
    qint64 bytes = 0;
    bool exists = false;
    bool isFolder = false;
    bool cancelled = false;
};

// Counts every file below a path, hidden ones included (a file counts as one).
// Runs in the calling thread; pass a flag to stop a long scan.
ScanResult scanPath(const QString &path, const std::atomic_bool *cancel = nullptr);

} // namespace scicat
