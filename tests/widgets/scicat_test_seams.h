// Included only by the test copy of the application that tests/CMakeLists.txt builds; see there.
// Without the environment of a test run the program stops instead of falling back to a real server or the real
// settings store.
#pragma once

#include <QString>
#include <cstdio>
#include <cstdlib>

namespace scicat_test {

inline QString requiredEnvironment(const char* name)
{
    if (!qEnvironmentVariableIsSet(name) || qEnvironmentVariableIsEmpty(name)) {
        std::fprintf(stderr, "scicat test build: %s is not set; run it through tests/parity/run_scenarios.py\n", name);
        std::abort();
    }
    return qEnvironmentVariable(name);
}

// base address of the fake SciCat, one prefix per server: <SCICAT_TEST_API>/<server>/api/
inline QString apiBase(const char* server)
{
    return requiredEnvironment("SCICAT_TEST_API") + "/" + QString::fromLatin1(server) + "/api/";
}

inline QString settingsFile()
{
    return requiredEnvironment("SCICAT_TEST_SETTINGS");
}

} // namespace scicat_test
