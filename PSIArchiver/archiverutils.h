#ifndef ARCHIVERUTILS_H
#define ARCHIVERUTILS_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <QDebug>

namespace utils {

template <typename Out>
void split(const std::string& s, char delim, Out result);
std::vector<std::string> split(const std::string& s, char delim);

}
inline QDebug operator<<(QDebug out, const std::string& str)
{
    out << QString::fromStdString(str);
    return out;
}

#define qDebugLine(X) (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).debug() << "[" <<  X << "]: ")

#ifdef QT_NO_DEBUG_MACRO
#undef qDebugLine
#define qDebugLine QT_NO_QDEBUG_MACRO
#endif

#endif // ARCHIVERUTILS_H


