if(SCICAT_BUNDLE_QT_RUNTIME AND SCICAT_INSTALL_SYSTEM_INTEGRATION AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(
        FILES scicat.desktop
        DESTINATION /usr/share/applications/
    )

    install(
        FILES scicat-logo-white.png
        DESTINATION /usr/share/icons/
    )
else()
    install(
        FILES scicat.desktop
        DESTINATION share/applications/
    )

    install(
        FILES scicat-logo-white.png
        DESTINATION share/icons/
    )
endif()
