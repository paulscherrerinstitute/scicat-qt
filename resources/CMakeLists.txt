set(SCICAT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../PSIArchiver/SciCat_logo_icon.svg")

if(SCICAT_BUNDLE_QT_RUNTIME AND SCICAT_INSTALL_SYSTEM_INTEGRATION AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(
        FILES scicat.desktop
        DESTINATION /usr/share/applications/
    )

    install(
        FILES "${SCICAT_ICON}"
        DESTINATION /usr/share/icons/
        RENAME scicat.svg
    )
else()
    install(
        FILES scicat.desktop
        DESTINATION share/applications/
    )

    install(
        FILES "${SCICAT_ICON}"
        DESTINATION share/icons/
        RENAME scicat.svg
    )
endif()
