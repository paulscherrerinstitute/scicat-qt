cmake_minimum_required(VERSION 3.5)

project(PSIArchiver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set ( RESOURCES  psiArchiverResources.qrc)
#add_definitions(-DQT_NO_DEBUG_OUTPUT)
add_definitions(-DKERBEROS)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
QT5_ADD_RESOURCES(RES_SOURCES ${RESOURCES})

if(ANDROID)
  add_library(PSIArchiver SHARED
    main.cpp
    dialog.cpp
    dialog.h
    dialog.ui
    filebrowser.ui
    login.ui
    archiverutils.cpp
    archiverutils.h
    pstream.h
    datasetmodel.cpp
    datasetmodel.h
    datasetbrowser.ui
  )
else()
  add_executable(PSIArchiver
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
    archiverutils.cpp
    archiverutils.h
    pstream.h
    datasetmodel.cpp
    datasetmodel.h
    datasetbrowser.cpp
    datasetbrowser.h
    datasetbrowser.ui
    logbrowser.cpp
    logbrowser.h
    logbrowser.ui
    settings.cpp
    settings.h
    settings.ui
    ${RES_SOURCES}
  )
endif()

target_link_libraries(PSIArchiver PRIVATE Qt5::Widgets)
