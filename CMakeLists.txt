cmake_minimum_required(VERSION 3.16)

set(SCICAT_VERSION "0.0.0" CACHE STRING "SciCat application version")
set(SCICAT_PACKAGE_RELEASE "1" CACHE STRING "SciCat RPM package release")
set(SCICAT_PACKAGE_GENERATORS "RPM" CACHE STRING "CPack package generators")
set(SCICAT_PACKAGE_SUFFIX "" CACHE STRING "Package filename suffix")
option(SCICAT_INSTALL_SYSTEM_INTEGRATION "Install system-wide Linux launcher and desktop resources" ON)

project(SciCat VERSION "${SCICAT_VERSION}" LANGUAGES CXX)

if(NOT SCICAT_PACKAGE_SUFFIX)
  set(SCICAT_PACKAGE_SUFFIX "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# build app
add_subdirectory(PSIArchiver)

# install resources
add_subdirectory(resources)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_PACKAGE_NAME "scicat")
  set(CPACK_PACKAGE_VENDOR "Paul Scherrer Institute")
  set(CPACK_PACKAGE_CONTACT "Paul Scherrer Institute")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SciCat GUI for archiving and retrieving data")
  set(CPACK_PACKAGE_VERSION "${SCICAT_VERSION}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${SCICAT_PACKAGE_SUFFIX}")

  set(CPACK_GENERATOR "${SCICAT_PACKAGE_GENERATORS}")
  set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
  set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
  set(CPACK_RPM_PACKAGE_RELEASE "${SCICAT_PACKAGE_RELEASE}")
  set(CPACK_RPM_PACKAGE_AUTOREQ ON)
  set(CPACK_RPM_PACKAGE_REQUIRES "libb2, double-conversion")
  set(CPACK_RPM_SPEC_MORE_DEFINE "%global __requires_exclude ^lib(Qt6.*|icu.*)\\\\.so.*$")

  include(CPack)
endif()
