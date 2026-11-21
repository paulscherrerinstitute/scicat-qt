qt_add_library(scicat_ui STATIC)

set_source_files_properties(Theme.qml Icons.qml Modals.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(scicat_ui
    URI SciCat.Ui
    QML_FILES
        AppButton.qml
        AppComboBox.qml
        AppDialog.qml
        AppTextArea.qml
        AppTextField.qml
        ColumnHeaderButton.qml
        EmptyState.qml
        FilterChip.qml
        FocusRing.qml
        FormField.qml
        Icon.qml
        IconButton.qml
        Icons.qml
        InlineMessage.qml
        KeyValue.qml
        LogView.qml
        Modals.qml
        NavButton.qml
        PageHeader.qml
        ScrollPage.qml
        SearchBox.qml
        SectionCard.qml
        SkeletonRows.qml
        StatusTag.qml
        Stepper.qml
        StoredTextField.qml
        SwitchRow.qml
        Theme.qml
        ToastHost.qml
)

target_link_libraries(scicat_ui
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)
