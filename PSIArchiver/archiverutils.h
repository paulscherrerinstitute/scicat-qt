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




#endif // ARCHIVERUTILS_H


