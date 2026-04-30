cmake_minimum_required(VERSION 3.16)

set(SCICAT_VERSION "0.0.0" CACHE STRING "SciCat application version")

project(SciCat VERSION "${SCICAT_VERSION}" LANGUAGES CXX)

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
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

  set(CPACK_GENERATOR "RPM")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
  set(CPACK_RPM_PACKAGE_RELEASE "1")
  set(CPACK_RPM_PACKAGE_AUTOREQ ON)

  include(CPack)
endif()
