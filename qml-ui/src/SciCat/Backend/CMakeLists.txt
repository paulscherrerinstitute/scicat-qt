qt_add_library(scicat_backend STATIC)

qt_add_qml_module(scicat_backend
    URI SciCat.Backend
    SOURCES
        activitymodel.cpp
        activitymodel.h
        archivedraft.cpp
        archivedraft.h
        backend.cpp
        backend.h
        copyqueue.cpp
        copyqueue.h
        datasetdetails.cpp
        datasetdetails.h
        datasetlistmodel.cpp
        datasetlistmodel.h
        fileentrymodel.cpp
        fileentrymodel.h
        ingestrun.cpp
        ingestrun.h
        inputdatasetmodel.cpp
        inputdatasetmodel.h
        metadatamodel.cpp
        metadatamodel.h
        retrievecontroller.cpp
        retrievecontroller.h
        session.cpp
        session.h
        settings.cpp
        settings.h
        workspace.cpp
        workspace.h
)

target_include_directories(scicat_backend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(scicat_backend
    PUBLIC
        scicat_core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Qml
)
