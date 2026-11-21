# The scenarios of the Widgets GUI (tests/parity in the repository root), played on this application.
qt_add_executable(scicat_qml_driver
    main.cpp
    scenarioplayer.cpp
    scenarioplayer.h
)

target_link_libraries(scicat_qml_driver
    PRIVATE
        scicat_appmain
        scicat_uiprobe
        Qt6::QuickPrivate
)

set(_parity "${PROJECT_SOURCE_DIR}/../tests/parity")
find_package(Python3 COMPONENTS Interpreter)
if(WIN32 OR NOT Python3_Interpreter_FOUND OR NOT EXISTS "${_parity}/run_scenarios.py")
    message(STATUS "SciCatQml: the scenario tests need Python 3 and tests/parity; the driver is built but not run by ctest")
    return()
endif()

# one test per scenario: ctest -R scenario.retrieve, ctest -j8
file(GLOB _scenarios CONFIGURE_DEPENDS "${_parity}/scenarios/*.json")
foreach(_scenario IN LISTS _scenarios)
    get_filename_component(_name "${_scenario}" NAME_WE)
    add_test(NAME "scenario.${_name}"
        COMMAND "${Python3_EXECUTABLE}" -I "${_parity}/run_scenarios.py" --name "${_name}"
                --driver "$<TARGET_FILE:scicat_qml_driver>" --unordered-reads
                --differences "${CMAKE_CURRENT_SOURCE_DIR}/differences"
                --work "${PROJECT_BINARY_DIR}/scenario-runs"
    )
endforeach()
