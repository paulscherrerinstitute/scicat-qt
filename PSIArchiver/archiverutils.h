#ifndef ARCHIVERUTILS_H
#define ARCHIVERUTILS_H

#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <tuple>
#include "pstream.h"
#include <QDebug>

namespace utils {

std::string system_call(const std::string& bash_cmd);

std::string extractFromJson(const std::string& jsonReply, const std::string& searchVal);

template <typename Out>
void split(const std::string& s, char delim, Out result);
std::vector<std::string> split(const std::string& s, char delim);

}

inline QDebug operator<<(QDebug out, const std::string& str)
{
    out << QString::fromStdString(str);
    return out;
}

#endif // ARCHIVERUTILS_H
