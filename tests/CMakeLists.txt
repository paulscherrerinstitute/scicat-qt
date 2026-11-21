# Scenario tests of the SciCat GUI: see tests/parity/README.md.
#
# The application is tested as it is. Its sources are copied into the build tree and two things are rewritten in the
# copy, so that a test run can never reach a real SciCat server or the user's own preferences:
#   - the three SciCat API addresses in DataCatalogue::serverList() -> the fake backend given by SCICAT_TEST_API
#   - every QSettings("PSI", "SciCat") -> the ini file given by SCICAT_TEST_SETTINGS
# Configuring fails when one of these rewrites no longer applies, instead of building a test that talks to the real
# servers or the real settings store. Nothing under PSIArchiver/ is modified.

find_package(Qt6 COMPONENTS Widgets Network Concurrent Svg REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(_app_src "${CMAKE_SOURCE_DIR}/PSIArchiver")
set(_app_copy "${CMAKE_CURRENT_BINARY_DIR}/scicat-app")
set(_seams "${CMAKE_CURRENT_SOURCE_DIR}/widgets/scicat_test_seams.h")

file(GLOB _app_files RELATIVE "${_app_src}" CONFIGURE_DEPENDS "${_app_src}/*")
set(_settings_sites 0)
set(_server_sites 0)
set(_driver_sources)
foreach(_file IN LISTS _app_files)
  if(IS_DIRECTORY "${_app_src}/${_file}" OR _file STREQUAL "CMakeLists.txt")
    continue()
  endif()
  if(NOT _file MATCHES "\\.(cpp|h)$")
    configure_file("${_app_src}/${_file}" "${_app_copy}/${_file}" COPYONLY)
  else()
    file(READ "${_app_src}/${_file}" _text)
    set(_patched "${_text}")

    if(_file STREQUAL "datacatalogue.cpp")
      foreach(_server IN ITEMS "production|https://dacat.psi.ch/api/" "qa|https://dacat-qa.psi.ch/api/"
                               "development|https://scicat.development.psi.ch/api/")
        string(REPLACE "|" ";" _server "${_server}")
        list(GET _server 0 _name)
        list(GET _server 1 _url)
        string(FIND "${_patched}" "\"${_url}\"" _at)
        if(_at EQUAL -1)
          message(FATAL_ERROR "tests: ${_url} not found in PSIArchiver/datacatalogue.cpp; update tests/CMakeLists.txt")
        endif()
        string(REPLACE "\"${_url}\"" "scicat_test::apiBase(\"${_name}\")" _patched "${_patched}")
        math(EXPR _server_sites "${_server_sites} + 1")
      endforeach()
    endif()

    string(REGEX MATCHALL "QSettings [A-Za-z_][A-Za-z0-9_]*\\(\"PSI\", \"SciCat\"\\)" _found "${_patched}")
    list(LENGTH _found _count)
    math(EXPR _settings_sites "${_settings_sites} + ${_count}")
    string(REGEX REPLACE "QSettings ([A-Za-z_][A-Za-z0-9_]*)\\(\"PSI\", \"SciCat\"\\)"
           "QSettings \\1(scicat_test::settingsFile(), QSettings::IniFormat)" _patched "${_patched}")
    string(FIND "${_patched}" "\"PSI\", \"SciCat\"" _left)
    if(NOT _left EQUAL -1)
      message(FATAL_ERROR "tests: PSIArchiver/${_file} opens the settings store in a way the test build cannot "
                          "redirect; update tests/CMakeLists.txt")
    endif()

    if(NOT _patched STREQUAL _text)
      set(_patched "#include \"${_seams}\" // added by tests/CMakeLists.txt\n${_patched}")
    endif()
    file(WRITE "${_app_copy}/${_file}.staged" "${_patched}")
    configure_file("${_app_copy}/${_file}.staged" "${_app_copy}/${_file}" COPYONLY)
    list(APPEND _driver_sources "${_app_copy}/${_file}")
  endif()
endforeach()
if(NOT _server_sites EQUAL 3 OR _settings_sites EQUAL 0)
  message(FATAL_ERROR "tests: expected 3 server addresses and at least one settings store, found ${_server_sites} and ${_settings_sites}")
endif()
message(STATUS "tests: redirected ${_server_sites} server addresses and ${_settings_sites} settings stores in the test copy")

foreach(_ui IN LISTS _app_files)
  if(_ui MATCHES "\\.(ui|qrc)$")
    list(APPEND _driver_sources "${_app_copy}/${_ui}")
  endif()
endforeach()

# the real main() of the application, plus the scenario driver, which starts itself with the QApplication
add_executable(scicat_widgets_driver ${_driver_sources} widgets/scenario_driver.cpp widgets/scicat_test_seams.h)
set_target_properties(scicat_widgets_driver PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
                      CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(scicat_widgets_driver PRIVATE "${_app_copy}")
# keep in line with PSIArchiver/CMakeLists.txt
target_compile_definitions(scicat_widgets_driver PRIVATE QT_NO_DEBUG_OUTPUT $<$<BOOL:${WIN32}>:WINDOWS_SPECIFIC_TOOLS>)
target_link_libraries(scicat_widgets_driver PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg)

# one test per scenario: ctest -R retrieve, ctest -j8
file(GLOB _scenarios CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/parity/scenarios/*.json")
foreach(_scenario IN LISTS _scenarios)
  get_filename_component(_name "${_scenario}" NAME_WE)
  add_test(NAME "scenario.${_name}"
           COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/parity/run_scenarios.py" --name "${_name}"
                   --driver "$<TARGET_FILE:scicat_widgets_driver>" --work "${CMAKE_CURRENT_BINARY_DIR}/scenario-runs")
endforeach()
