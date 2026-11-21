qt_add_library(scicat_core STATIC
    apiclient.cpp
    apiclient.h
    appsettings.cpp
    appsettings.h
    catalog.cpp
    catalog.h
    clitool.cpp
    clitool.h
    datasetmetadata.cpp
    datasetmetadata.h
    filescan.cpp
    filescan.h
    formatting.cpp
    formatting.h
    processrunner.cpp
    processrunner.h
    proposalstore.cpp
    proposalstore.h
    servers.cpp
    servers.h
)

target_include_directories(scicat_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(scicat_core
    PUBLIC
        Qt6::Core
        Qt6::Network
)
