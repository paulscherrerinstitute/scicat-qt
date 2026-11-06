cmake_minimum_required(VERSION 3.16)

project(SciCat LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RESOURCES psiArchiverResources.qrc)

add_definitions(-DQT_NO_DEBUG_OUTPUT)
add_definitions(-DSKIP_KERBEROS_AUTH)
#add_definitions(-DNO_DATACATALOGUE_TOOLS)

find_package(Qt6 COMPONENTS Widgets Network Concurrent Svg REQUIRED)

option(SCICAT_BUNDLE_QT_RUNTIME "Install a bundled Qt runtime with SciCat" OFF)
set(SCICAT_BUNDLED_QT_LIBRARY_DIR "" CACHE PATH "Qt library directory to bundle")
set(SCICAT_BUNDLED_QT_PLUGIN_DIR "" CACHE PATH "Qt plugin directory to bundle")

set(SCICAT_SOURCES
  pch.h
  main.cpp
  dialog.cpp
  dialog.h
  dialog.ui
  filebrowser.cpp
  filebrowser.h
  filebrowser.ui
  login.cpp
  login.h
  login.ui
  datasetmodel.cpp
  datasetmodel.h
  datasetbrowser.cpp
  datasetbrowser.h
  datasetbrowser.ui
  datasetingestor.cpp
  datasetingestor.h
  logbrowser.cpp
  logbrowser.h
  logbrowser.ui
  settings.cpp
  settings.h
  settings.ui
  filecountcontainer.cpp
  filecountcontainer.h
  filedialogfilelisting.cpp
  filedialogfilelisting.h
  creationlocationcreator.cpp
  creationlocationcreator.h
  creationlocationcreator.ui
  networkmanager.cpp
  networkmanager.h
  datacatalogue.cpp
  datacatalogue.h
  datacatalogueExperiment.cpp
  datacatalogueExperiment.h
  datacatalogueUser.cpp
  datacatalogueUser.h
  datacatalogueClasses.h
  datasetretriever.cpp
  datasetretriever.h
  downloadmanager.cpp
  downloadmanager.h
  processhandler.cpp
  processhandler.h
  messagehandlergui.cpp
  messagehandlergui.h
  ${RESOURCES}
)

if(WIN32)
  add_definitions(-DWINDOWS_SPECIFIC_TOOLS)
  add_executable(SciCat WIN32 ${SCICAT_SOURCES} SciCat.rc)
elseif(APPLE)
  set(SCICAT_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/SciCat.icns")
  set_source_files_properties("${SCICAT_MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  add_executable(SciCat MACOSX_BUNDLE ${SCICAT_SOURCES} "${SCICAT_MACOS_ICON}")
  set_target_properties(SciCat PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "SciCat.icns"
    MACOSX_BUNDLE_BUNDLE_NAME "SciCat"
    MACOSX_BUNDLE_GUI_IDENTIFIER "ch.psi.scicat"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${SCICAT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${SCICAT_VERSION}"
  )
else()
  add_executable(SciCat ${SCICAT_SOURCES})
endif()
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.5")
    target_precompile_headers(SciCat PUBLIC pch.h)
else()
    message("Please consider upgrading to CMake version > 3.16 for faster compilation.")
endif()

target_link_libraries(SciCat PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Svg)

include("${CMAKE_SOURCE_DIR}/cmake/SciCatCli.cmake")

if(SCICAT_BUNDLE_QT_RUNTIME AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(NOT IS_DIRECTORY "${SCICAT_BUNDLED_QT_LIBRARY_DIR}")
    message(FATAL_ERROR "SCICAT_BUNDLED_QT_LIBRARY_DIR must point to an existing Qt library directory")
  endif()
  if(NOT IS_DIRECTORY "${SCICAT_BUNDLED_QT_PLUGIN_DIR}")
    message(FATAL_ERROR "SCICAT_BUNDLED_QT_PLUGIN_DIR must point to an existing Qt plugin directory")
  endif()

  set_target_properties(SciCat PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
  )

  install(
    DIRECTORY "${SCICAT_BUNDLED_QT_LIBRARY_DIR}/"
    DESTINATION lib
    FILES_MATCHING PATTERN "libQt6*.so*"
  )

  install(
    DIRECTORY "${SCICAT_BUNDLED_QT_PLUGIN_DIR}/"
    DESTINATION plugins
    FILES_MATCHING PATTERN "*.so"
  )

  install(
    FILES "${CMAKE_SOURCE_DIR}/cmake/qt.conf"
    DESTINATION bin
  )

  if(SCICAT_INSTALL_SYSTEM_INTEGRATION)
    install(
      PROGRAMS "${CMAKE_SOURCE_DIR}/cmake/SciCatBundledQt.sh"
      DESTINATION /usr/bin
      RENAME SciCat
    )
  endif()
endif()

install(TARGETS SciCat
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin
)
