#pragma once

#include <functional>

class Backend;
class QQmlApplicationEngine;

namespace scicat {

// Starts the application: backend, QML engine, main window. "ready" runs once the window exists;
// the end-to-end test uses it to attach its driver.
int runApplication(int argc, char *argv[], const std::function<void(QQmlApplicationEngine &, Backend &)> &ready = {});

} // namespace scicat
