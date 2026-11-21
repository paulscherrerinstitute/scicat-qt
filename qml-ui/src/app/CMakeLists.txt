# everything both executables share: start-up code, the style configuration and the three QML modules
qt_add_library(scicat_appmain STATIC
    appmain.cpp
    appmain.h
)

target_compile_definitions(scicat_appmain
    PRIVATE
        SCICAT_VERSION="${PROJECT_VERSION}"
)

target_include_directories(scicat_appmain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_add_resources(scicat_appmain "scicat_controls_conf"
    PREFIX "/"
    FILES
        qtquickcontrols2.conf
)

target_link_libraries(scicat_appmain
    PUBLIC
        scicat_backend
        scicat_backendplugin
        scicat_uiplugin
        scicat_appplugin
        Qt6::Quick
        Qt6::QuickControls2
        # the logo is an SVG file: with Qt Svg linked, the deployment tools ship the plugin that reads it
        Qt6::Svg
)

set(scicat_macos_icon "${CMAKE_CURRENT_SOURCE_DIR}/../../../PSIArchiver/SciCat.icns")

if(APPLE)
    set_source_files_properties("${scicat_macos_icon}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    qt_add_executable(scicat_qml MACOSX_BUNDLE
        main.cpp
        "${scicat_macos_icon}"
    )
    set_target_properties(scicat_qml PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "SciCat.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "SciCat QML"
        MACOSX_BUNDLE_GUI_IDENTIFIER "ch.psi.scicat.qml"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
elseif(WIN32)
    qt_add_executable(scicat_qml WIN32
        main.cpp
    )
else()
    qt_add_executable(scicat_qml
        main.cpp
    )
endif()

set_target_properties(scicat_qml PROPERTIES
    OUTPUT_NAME "SciCatQml"
)

target_link_libraries(scicat_qml
    PRIVATE
        scicat_appmain
)

install(TARGETS scicat_qml
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
