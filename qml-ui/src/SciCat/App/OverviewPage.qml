pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Start page: choose the pgroup, see its proposal, then pick what to do.
ScrollPage {
    id: page

    readonly property Workspace workspace: Backend.workspace
    // the text field inside the box: with the focus on the box itself, typing would not arrive
    readonly property Item initialFocusItem: groupBox.contentItem
    property string groupProblem
    property bool piProblemsShown: false

    signal archiveRequested()
    signal retrieveRequested()

    function choose(group: string) {
        groupProblem = workspace.problemWith(group)
        if (groupProblem === "") {
            piProblemsShown = false
            workspace.select(group)
        }
    }

    function syncGroupBox() {
        const index = groupBox.find(workspace.pgroup)
        groupBox.currentIndex = index
        groupBox.editText = workspace.pgroup
    }

    // A pgroup typed without Return counts once the box is left, like any other field: the box must not show
    // one pgroup while the page works on another. An emptied box goes back to the pgroup in use.
    function commitTyped() {
        const typed = groupBox.editText.trim()
        if (typed === "") {
            groupProblem = ""
            syncGroupBox()
        } else if (typed !== workspace.pgroup) {
            choose(typed)
        }
    }

    function startArchive() {
        if (!workspace.piComplete) {
            piProblemsShown = true
            // the first field that is missing or wrong
            const field = firstNameField.text.trim() === "" ? firstNameField : lastNameField.text.trim() === "" ? lastNameField : emailField
            field.forceActiveFocus()
            return
        }
        archiveRequested()
    }

    Component.onCompleted: syncGroupBox()

    Connections {
        target: page.workspace

        function onPgroupChanged() {
            page.syncGroupBox()
        }
    }

    Item {
        implicitWidth: page.availableWidth
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
                title: qsTr("Which pgroup are you working on?")
                subtitle: qsTr("Everything you archive or retrieve belongs to one pgroup.")

                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Pgroup")
                    problem: page.groupProblem
                    help: Backend.session.groups.length === 1 ? qsTr("You are a member of one group.")
                        : qsTr("Pick one of your %1 groups, or type it and press Return.").arg(Backend.session.groups.length)

                    AppComboBox {
                        id: groupBox

                        Layout.fillWidth: true
                        objectName: "groupBox"
                        editable: true
                        selectTextByMouse: true
                        model: Backend.session.groups
                        focus: true
                        Accessible.name: qsTr("Pgroup")
                        onActivated: index => page.choose(textAt(index))
                        onAccepted: page.choose(editText)
                        // not while its list is open: the choice made there says which pgroup is meant
                        onActiveFocusChanged: {
                            if (!activeFocus && !popup.visible)
                                page.commitTyped()
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapS
                    visible: page.workspace.recentGroups.length > 1

                    Label {
                        text: qsTr("Recent")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                    }
                    Repeater {
                        model: page.workspace.recentGroups

                        FilterChip {
                            required property string modelData

                            objectName: "recentGroup"
                            text: modelData
                            checked: modelData === page.workspace.pgroup
                            onClicked: page.choose(modelData)
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gapM
                visible: page.workspace.state === Workspace.Loading

                BusyIndicator {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    running: page.workspace.state === Workspace.Loading && page.visible
                }
                Label {
                    Layout.fillWidth: true
                    objectName: "proposalLoading"
                    text: qsTr("Looking up the proposal of %1…").arg(page.workspace.pgroup)
                    color: Theme.textSecondary
                }
            }

            InlineMessage {
                Layout.fillWidth: true
                objectName: "proposalProblem"
                visible: page.workspace.state === Workspace.Failed
                tone: "error"
                title: qsTr("The proposal could not be loaded")
                text: page.workspace.errorText
                actionText: qsTr("Try again")
                onActionTriggered: page.workspace.reload()
            }

            SectionCard {
                Layout.fillWidth: true
                objectName: "proposalCard"
                visible: page.workspace.state === Workspace.Ready && page.workspace.source === Workspace.FromCatalog
                title: qsTr("Proposal")

                KeyValue {
                    Layout.fillWidth: true
                    label: qsTr("Title")
                    value: page.workspace.proposalTitle
                    labelWidth: 170
                }
                KeyValue {
                    Layout.fillWidth: true
                    label: qsTr("Principal investigator")
                    // a proposal may lack the name or the email address: what is there is shown, without empty brackets
                    value: page.workspace.owner.trim() === "" ? page.workspace.piEmail
                        : page.workspace.piEmail === "" ? page.workspace.owner.trim()
                                                        : qsTr("%1 (%2)").arg(page.workspace.owner.trim()).arg(page.workspace.piEmail)
                    labelWidth: 170
                }
                KeyValue {
                    Layout.fillWidth: true
                    label: qsTr("Instrument")
                    value: page.workspace.instrument
                    labelWidth: 170
                }
                KeyValue {
                    Layout.fillWidth: true
                    label: qsTr("Proposal ID")
                    value: page.workspace.proposalId
                    mono: true
                    labelWidth: 170
                }
                KeyValue {
                    Layout.fillWidth: true
                    label: qsTr("Abstract")
                    value: page.workspace.proposalAbstract
                    visible: page.workspace.proposalAbstract !== ""
                    labelWidth: 170
                }
            }

            SectionCard {
                Layout.fillWidth: true
                objectName: "piCard"
                // also for a proposal in SciCat that does not name its principal investigator completely
                visible: page.workspace.state === Workspace.Ready && page.workspace.piEditable
                title: qsTr("Principal investigator")

                InlineMessage {
                    Layout.fillWidth: true
                    objectName: "piNote"
                    tone: "info"
                    text: page.workspace.source === Workspace.FromCatalog
                        ? qsTr("The proposal of %1 in SciCat does not name its principal investigator completely. Archiving needs to know, so fill in what is missing here. Retrieving works without it.").arg(page.workspace.pgroup)
                        : page.workspace.source === Workspace.FromSavedFile
                        ? qsTr("SciCat has no proposal for %1. These details were entered on this computer last time; change them if they are no longer right.").arg(page.workspace.pgroup)
                        : qsTr("SciCat has no proposal for %1. Archiving needs to know the principal investigator, so enter the details here. Retrieving works without them.").arg(page.workspace.pgroup)
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("First name")
                    problem: page.piProblemsShown && firstNameField.text.trim() === "" ? qsTr("Enter the first name of the principal investigator.") : ""

                    AppTextField {
                        id: firstNameField

                        Layout.fillWidth: true
                        objectName: "piFirstName"
                        text: page.workspace.piFirstName
                        Accessible.name: qsTr("First name of the principal investigator")
                        onTextEdited: page.workspace.piFirstName = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Last name")
                    problem: page.piProblemsShown && lastNameField.text.trim() === "" ? qsTr("Enter the last name of the principal investigator.") : ""

                    AppTextField {
                        id: lastNameField

                        Layout.fillWidth: true
                        objectName: "piLastName"
                        text: page.workspace.piLastName
                        Accessible.name: qsTr("Last name of the principal investigator")
                        onTextEdited: page.workspace.piLastName = text
                    }
                }
                FormField {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Theme.formWidth
                    label: qsTr("Email")
                    problem: !page.piProblemsShown || page.workspace.piEmailValid ? ""
                        : emailField.text.trim() === "" ? qsTr("Enter the email address of the principal investigator.") : qsTr("This is not an email address.")
                    help: emailField.text.trim() !== "" && !page.workspace.piEmailValid ? qsTr("This does not look like an email address yet.")
                        : Backend.settings.saveProposalInfo ? qsTr("Kept on this computer for the next time you use %1.").arg(page.workspace.pgroup) : ""

                    AppTextField {
                        id: emailField

                        Layout.fillWidth: true
                        objectName: "piEmail"
                        text: page.workspace.piEmail
                        inputMethodHints: Qt.ImhEmailCharactersOnly
                        Accessible.name: qsTr("Email of the principal investigator")
                        onTextEdited: page.workspace.piEmail = text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gapL
                visible: page.workspace.state === Workspace.Ready

                ChoiceTile {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    objectName: "archiveTile"
                    iconName: "upload"
                    title: qsTr("Archive data")
                    description: qsTr("Register files of %1 in the catalogue and send them to tape.").arg(page.workspace.pgroup)
                    onClicked: page.startArchive()
                }
                ChoiceTile {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    objectName: "retrieveTile"
                    iconName: "download"
                    title: qsTr("Retrieve data")
                    description: qsTr("Get archived datasets of %1 back to a folder on this computer.").arg(page.workspace.pgroup)
                    onClicked: page.retrieveRequested()
                }
            }
        }
    }
}
