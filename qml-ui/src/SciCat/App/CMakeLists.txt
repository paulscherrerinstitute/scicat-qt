qt_add_library(scicat_app STATIC)

set(scicat_logo "${CMAKE_CURRENT_SOURCE_DIR}/../../../../PSIArchiver/SciCat_logo_icon.svg")

set_source_files_properties("${scicat_logo}" PROPERTIES
    QT_RESOURCE_ALIAS icons/scicat-logo.svg
)

qt_add_qml_module(scicat_app
    URI SciCat.App
    DEPENDENCIES
        TARGET scicat_backend
        TARGET scicat_ui
    QML_FILES
        ActivityPage.qml
        ArchiveDetailsStep.qml
        ArchiveFilesStep.qml
        ArchivePage.qml
        ArchiveReviewStep.qml
        ArchiveRunView.qml
        BlockedPage.qml
        ChoiceTile.qml
        DatasetDetailsPanel.qml
        DatasetRow.qml
        HelpPage.qml
        HelpStep.qml
        LoginPage.qml
        Main.qml
        MetadataEditor.qml
        OverviewPage.qml
        RetrievePage.qml
        SettingsForm.qml
        Shell.qml
    RESOURCES
        "${scicat_logo}"
)

target_link_libraries(scicat_app
    PRIVATE
        scicat_backend
        Qt6::Quick
        Qt6::QuickControls2
)
