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

find_package(Qt6 COMPONENTS Widgets Network Concurrent REQUIRED)

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
  add_executable(SciCat WIN32 ${SCICAT_SOURCES})
elseif(APPLE)
  add_executable(SciCat MACOSX_BUNDLE ${SCICAT_SOURCES})
  set_target_properties(SciCat PROPERTIES
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

target_link_libraries(SciCat PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent)

install(TARGETS SciCat
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin
)
