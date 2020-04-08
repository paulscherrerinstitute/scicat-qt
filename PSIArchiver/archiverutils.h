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


namespace utils {

std::string system_call(const std::string& bash_cmd);

std::string extractFromJson(const std::string& jsonReply, const std::string& searchVal);

}

#endif // ARCHIVERUTILS_H
