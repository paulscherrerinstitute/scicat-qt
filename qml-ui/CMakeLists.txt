cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED SCICAT_VERSION)
    set(SCICAT_VERSION "0.0.0" CACHE STRING "SciCat application version")
endif()

project(SciCatQml
    VERSION "${SCICAT_VERSION}"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    Network
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    Svg
)

qt_standard_project_setup(REQUIRES 6.8)

option(SCICAT_QML_TESTS "Build the tests of the Qt Quick GUI" ON)

add_subdirectory(src/core)
add_subdirectory(src/SciCat/Backend)
add_subdirectory(src/SciCat/Ui)
add_subdirectory(src/SciCat/App)
add_subdirectory(src/app)

if(SCICAT_QML_TESTS)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
