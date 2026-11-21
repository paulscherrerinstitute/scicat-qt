pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Step 2: what the dataset is. One page from top to bottom, in the order it is filled in.
ScrollPage {
    id: step

    readonly property ArchiveDraft draft: Backend.archive
    property bool customInstrument: false
    property string customProblem
    // why the last import or export of the scientific metadata did not work; stays until dismissed
    property string metadataFileProblem

    signal notify(string message)
    // a change that can be taken back: the page shows the message with an "Undo" that calls undo()
    signal undoOffered(string message, var undo)

    function syncInstrument() {
        instrumentBox.currentIndex = instrumentBox.find(draft.creationLocation)
    }

    // An import replaces what is there, so what was there can be brought back.
    function importMetadata(file: url) {
        const replaced = !draft.metadata.empty
        metadataFileProblem = draft.metadata.importFrom(file)
        if (metadataFileProblem !== "")
            return
        if (replaced)
            step.undoOffered(qsTr("The imported file replaced the scientific metadata."), () => step.draft.metadata.undoImport())
        else
            step.notify(qsTr("Scientific metadata imported."))
    }

    // Hands over a name that is still being typed. A field keeps the focus when a button is clicked on some
    // platforms, so "Next" has to ask for it.
    function commitPending() {
        nameField.commit()
        ownerField.commit()
        ownerEmailField.commit()
        principalInvestigatorField.commit()
        proposalIdField.commit()
    }

    function syncOwnerGroup() {
        ownerGroupBox.currentIndex = ownerGroupBox.find(draft.ownerGroup)
    }

    function choosePicked() {
        pickerSearch.clear()
        draft.openPicker()
        pickerDialog.open()
    }

    // puts the cursor into the first field that has a problem; the page scrolls there with it
    function focusFirstProblem() {
        const field = draft.datasetNameProblem !== "" ? nameField
            : draft.raw && draft.instrumentProblem !== "" ? instrumentBox
            : !draft.raw && draft.investigatorProblem !== "" ? investigatorField
            : draft.ownerProblem !== "" ? ownerField
            : draft.ownerEmailProblem !== "" ? ownerEmailField
            : draft.ownerGroupProblem !== "" ? ownerGroupBox : null
        if (field)
            field.forceActiveFocus(Qt.TabFocusReason)
        else if (draft.raw && draft.metadataProblem !== "")
            metadataEditor.focusFirstProblem()
        // also when that field had the cursor already and the page was scrolled away from it
        step.revealFocused()
    }

    objectName: "archiveDetails"
    Component.onCompleted: {
        syncInstrument()
        syncOwnerGroup()
    }

    Connections {
        target: step.draft

        function onDetailsChanged() {
            step.syncInstrument()
        }
        function onInstrumentsChanged() {
            step.syncInstrument()
        }
        function onOwnershipChanged() {
            step.syncOwnerGroup()
        }
        // what this page keeps beside the draft belongs to the dataset that was there before
        function onStartedNew() {
            step.metadataFileProblem = ""
            step.customProblem = ""
            step.customInstrument = false
            instituteField.clear()
            facilityField.clear()
            instrumentField.clear()
            step.scrollToTop()
        }
    }

    Item {
        implicitWidth: step.availableWidth
        implicitHeight: column.implicitHeight + 2 * Theme.gapXl

        ColumnLayout {
            id: column

            anchors.top: parent.top
            anchors.topMargin: Theme.gapXl
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width - 2 * Theme.gapXl, 820)
            spacing: Theme.gapL

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Dataset")

                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Kind of data")

                    // one under the other: each reads as a whole line, however long its wording gets
                    ColumnLayout {
                        spacing: 0

                        RadioButton {
                            objectName: "rawChoice"
                            text: qsTr("Raw: measured at an instrument")
                            checked: step.draft.raw
                            onToggled: if (checked) step.draft.raw = true
                        }
                        RadioButton {
                            objectName: "derivedChoice"
                            text: qsTr("Derived: computed from other datasets")
                            checked: !step.draft.raw
                            onToggled: if (checked) step.draft.raw = false
                        }
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Name")
                    problem: step.draft.datasetNameProblem
                    help: qsTr("For example catalyst_analysis_ID23. Spaces become underscores.")

                    StoredTextField {
                        id: nameField

                        Layout.fillWidth: true
                        objectName: "datasetName"
                        stored: step.draft.datasetName
                        Accessible.name: qsTr("Name of the dataset")
                        onCommitted: text => step.draft.datasetName = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Description")
                    optional: true
                    help: qsTr("What someone finding this dataset in a year should know, in a sentence or two.")

                    AppTextArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        objectName: "datasetDescription"
                        text: step.draft.description
                        wrapMode: TextEdit.Wrap
                        Accessible.name: qsTr("Description")
                        onTextChanged: if (activeFocus) step.draft.description = text
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                visible: step.draft.raw
                title: qsTr("Where it was measured")

                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Instrument")
                    problem: step.draft.instrumentProblem
                    help: Backend.workspace.instrument !== "" && step.draft.creationLocation === Backend.workspace.instrument
                        ? qsTr("Taken from the proposal.") : qsTr("Type to jump to an entry.")

                    AppComboBox {
                        id: instrumentBox

                        Layout.fillWidth: true
                        objectName: "instrumentBox"
                        model: step.draft.instruments
                        Accessible.name: qsTr("Instrument")
                        onActivated: index => step.draft.creationLocation = textAt(index)
                    }
                }
                AppButton {
                    Layout.topMargin: -Theme.gapS
                    objectName: "customInstrumentToggle"
                    text: step.customInstrument ? qsTr("Hide the fields below") : qsTr("Not in the list? Add an instrument")
                    variant: AppButton.Plain
                    onClicked: step.customInstrument = !step.customInstrument
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM
                    visible: step.customInstrument

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        label: qsTr("Institute")
                        problem: step.customProblem === "institute" ? qsTr("Enter the institute, for example PSI.") : ""

                        AppTextField {
                            id: instituteField

                            Layout.fillWidth: true
                            objectName: "customInstitute"
                            Accessible.name: qsTr("Institute")
                        }
                    }
                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        label: qsTr("Facility")
                        optional: true

                        AppTextField {
                            id: facilityField

                            Layout.fillWidth: true
                            objectName: "customFacility"
                            Accessible.name: qsTr("Facility")
                        }
                    }
                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        label: qsTr("Instrument")
                        problem: step.customProblem === "instrument" ? qsTr("Enter the instrument, for example TOMCAT.") : ""

                        AppTextField {
                            id: instrumentField

                            Layout.fillWidth: true
                            objectName: "customInstrumentName"
                            Accessible.name: qsTr("Instrument name")
                        }
                    }
                    AppButton {
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 24
                        objectName: "customInstrumentAdd"
                        text: qsTr("Add instrument")
                        onClicked: {
                            step.customProblem = step.draft.addInstrument(instituteField.text, facilityField.text, instrumentField.text)
                            if (step.customProblem === "") {
                                step.customInstrument = false
                                instituteField.clear()
                                facilityField.clear()
                                instrumentField.clear()
                            }
                        }
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                visible: !step.draft.raw
                title: qsTr("How it was made")

                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Investigator")
                    problem: step.draft.investigatorProblem
                    help: qsTr("Email address of the person who made this dataset.")

                    AppTextField {
                        id: investigatorField

                        Layout.fillWidth: true
                        objectName: "investigator"
                        text: step.draft.investigator
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        Accessible.name: qsTr("Investigator")
                        onTextEdited: step.draft.investigator = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Software used")
                    optional: true
                    help: qsTr("A name, or better a link to the exact version, for example a commit in GitLab.")

                    AppTextArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                        objectName: "usedSoftware"
                        text: step.draft.usedSoftware
                        wrapMode: TextEdit.Wrap
                        Accessible.name: qsTr("Software used")
                        onTextChanged: if (activeFocus) step.draft.usedSoftware = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Input datasets")
                    optional: true
                    help: step.draft.inputDatasets.count === 0 ? qsTr("The archived datasets this one was computed from.") : ""

                    Repeater {
                        model: step.draft.inputDatasets

                        RowLayout {
                            id: inputRow

                            required property int index
                            required property string name
                            required property string pid

                            Layout.fillWidth: true
                            spacing: Theme.gapS
                            objectName: "inputDatasetRow"

                            Icon {
                                name: "database"
                                size: 18
                                color: Theme.textSecondary
                            }
                            Label {
                                // up to half of the row; a longer name is cut and the ID keeps its place
                                Layout.maximumWidth: inputRow.width / 2
                                text: inputRow.name
                                color: Theme.text
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                text: inputRow.pid
                                color: Theme.textSecondary
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.fontSmall
                                elide: Text.ElideMiddle
                            }
                            IconButton {
                                iconName: "close"
                                label: qsTr("Remove %1 from the input datasets").arg(inputRow.name)
                                onClicked: step.draft.inputDatasets.removeAt(inputRow.index)
                            }
                        }
                    }
                    AppButton {
                        objectName: "chooseInputDatasets"
                        text: qsTr("Choose datasets…")
                        iconName: "database"
                        onClicked: step.choosePicked()
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                objectName: "ownerCard"
                title: qsTr("Owner")
                subtitle: qsTr("Filled in from the proposal of %1. Change it only where this dataset differs.").arg(Backend.workspace.pgroup)

                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Owner")
                    problem: step.draft.ownerProblem

                    StoredTextField {
                        id: ownerField

                        Layout.fillWidth: true
                        objectName: "ownerName"
                        stored: step.draft.owner
                        Accessible.name: qsTr("Owner")
                        onCommitted: text => step.draft.owner = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Owner email")
                    problem: step.draft.ownerEmailProblem
                    help: qsTr("Empty the field to go back to the address the proposal gives.")

                    StoredTextField {
                        id: ownerEmailField

                        Layout.fillWidth: true
                        objectName: "ownerEmail"
                        stored: step.draft.ownerEmail
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        Accessible.name: qsTr("Owner email")
                        onCommitted: text => step.draft.ownerEmail = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Owner group")
                    problem: step.draft.ownerGroupProblem
                    help: qsTr("One of your pgroups. Its members can see the dataset.")

                    AppComboBox {
                        id: ownerGroupBox

                        Layout.fillWidth: true
                        objectName: "ownerGroupBox"
                        model: Backend.session.groups
                        Accessible.name: qsTr("Owner group")
                        onActivated: index => step.draft.ownerGroup = textAt(index)
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    visible: step.draft.raw
                    label: qsTr("Principal investigator")
                    help: qsTr("An email address or a name. Empty the field to go back to the one the proposal gives.")

                    StoredTextField {
                        id: principalInvestigatorField

                        Layout.fillWidth: true
                        objectName: "principalInvestigator"
                        stored: step.draft.principalInvestigator
                        Accessible.name: qsTr("Principal investigator")
                        onCommitted: text => step.draft.principalInvestigator = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Proposal ID")
                    optional: true

                    StoredTextField {
                        id: proposalIdField

                        Layout.fillWidth: true
                        objectName: "proposalId"
                        stored: step.draft.proposalId
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontSmall
                        Accessible.name: qsTr("Proposal ID")
                        onCommitted: text => step.draft.proposalId = text
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                objectName: "metadataCard"
                visible: step.draft.raw
                title: qsTr("Scientific metadata")
                subtitle: qsTr("What describes the measurement. Numbers stay numbers, so they can be searched by range in SciCat.")

                headerItems: [
                    AppButton {
                        objectName: "metadataImport"
                        text: qsTr("Import…")
                        variant: AppButton.Plain
                        onClicked: importDialog.open()
                    },
                    AppButton {
                        objectName: "metadataExport"
                        text: qsTr("Export…")
                        variant: AppButton.Plain
                        enabled: !step.draft.metadata.empty
                        onClicked: exportDialog.open()
                    }
                ]

                InlineMessage {
                    Layout.fillWidth: true
                    objectName: "metadataProblem"
                    visible: step.draft.metadataProblem !== ""
                    tone: "error"
                    text: step.draft.metadataProblem
                }
                InlineMessage {
                    Layout.fillWidth: true
                    objectName: "metadataFileProblem"
                    visible: step.metadataFileProblem !== ""
                    tone: "error"
                    text: step.metadataFileProblem
                    dismissible: true
                    onDismissed: step.metadataFileProblem = ""
                }
                MetadataEditor {
                    id: metadataEditor

                    Layout.fillWidth: true
                    metadata: step.draft.metadata
                    showAllProblems: step.draft.detailProblemsShown
                }
            }
        }
    }

    FileDialog {
        id: importDialog

        title: qsTr("Import scientific metadata")
        nameFilters: [qsTr("JSON files (*.json)")]
        onAccepted: step.importMetadata(selectedFile)
    }
    FileDialog {
        id: exportDialog

        title: qsTr("Export scientific metadata")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON files (*.json)")]
        onAccepted: {
            step.metadataFileProblem = step.draft.metadata.exportTo(selectedFile)
            if (step.metadataFileProblem === "")
                step.notify(qsTr("Scientific metadata exported."))
        }
    }

    AppDialog {
        id: pickerDialog

        objectName: "pickerDialog"
        width: Math.min(parent.width - 2 * Theme.gapXl, 680)
        height: Math.min(parent.height - 2 * Theme.gapXl, 560)
        title: qsTr("Input datasets from %1").arg(Backend.workspace.pgroup)
        initialFocusItem: pickerSearch

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.gapM

            SearchBox {
                id: pickerSearch

                Layout.fillWidth: true
                objectName: "pickerSearch"
                label: qsTr("Search by name or ID")
                onSearchRequested: text => step.draft.picker.search = text
            }
            InlineMessage {
                Layout.fillWidth: true
                visible: step.draft.pickerNote !== ""
                tone: step.draft.pickerState === ArchiveDraft.PickerFailed ? "error" : "warning"
                text: step.draft.pickerNote
                actionText: step.draft.pickerState === ArchiveDraft.PickerFailed ? qsTr("Try again") : ""
                onActionTriggered: step.draft.loadPicker()
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radiusSmall
                color: Theme.surface
                border.width: 1
                border.color: Theme.outline

                SkeletonRows {
                    anchors.fill: parent
                    anchors.margins: 1
                    visible: step.draft.pickerState === ArchiveDraft.PickerLoading
                    rows: 7
                }
                EmptyState {
                    anchors.fill: parent
                    visible: step.draft.pickerState === ArchiveDraft.PickerReady && step.draft.picker.count === 0
                    iconName: "database"
                    title: step.draft.picker.filtered ? qsTr("Nothing matches \"%1\"").arg(step.draft.picker.search) : qsTr("%1 has no datasets yet").arg(Backend.workspace.pgroup)
                    actionText: step.draft.picker.filtered ? qsTr("Clear the search") : ""
                    onActionTriggered: pickerSearch.clear()
                }
                ListView {
                    id: pickerList

                    anchors.fill: parent
                    anchors.margins: 1
                    objectName: "pickerList"
                    visible: step.draft.pickerState === ArchiveDraft.PickerReady && step.draft.picker.count > 0
                    clip: true
                    model: step.draft.picker
                    activeFocusOnTab: true
                    Accessible.role: Accessible.List
                    Accessible.name: qsTr("Datasets")
                    ScrollBar.vertical: ScrollBar {}

                    delegate: CheckDelegate {
                        id: pickRow

                        required property int index
                        required property string name
                        required property string pid
                        required property bool selected

                        width: ListView.view.width
                        objectName: "pickerRow"
                        text: name
                        checked: selected
                        Accessible.name: qsTr("%1, %2").arg(name).arg(pid)
                        onToggled: step.draft.togglePicked(index)
                    }
                }
                FocusRing {
                    shown: pickerList.activeFocus
                    inset: 0
                }
            }
        }
        footer: RowLayout {
            spacing: Theme.gapS

            Label {
                Layout.leftMargin: Theme.gapXl
                Layout.fillWidth: true
                objectName: "pickerCount"
                text: step.draft.pickerState === ArchiveDraft.PickerReady ? qsTr("%1 listed").arg(Backend.formatCount(step.draft.picker.count)) : ""
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
            AppButton {
                Layout.topMargin: Theme.gapM
                Layout.bottomMargin: Theme.gapM
                text: qsTr("Cancel")
                variant: AppButton.Plain
                onClicked: pickerDialog.close()
            }
            AppButton {
                Layout.margins: Theme.gapM
                objectName: "pickerAdd"
                text: step.draft.pickedCount === 1 ? qsTr("Add 1 dataset") : qsTr("Add %1 datasets").arg(step.draft.pickedCount)
                variant: AppButton.Filled
                enabled: step.draft.pickedCount > 0
                onClicked: {
                    step.draft.addPicked()
                    pickerDialog.close()
                }
            }
        }
    }
}
