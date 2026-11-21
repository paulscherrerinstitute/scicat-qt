import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Preferences. Every switch takes effect at once; there is nothing to save.
ScrollPage {
    id: form

    readonly property Settings settings: Backend.settings
    // the server is chosen on the sign-in page; here it is shown with the way to change it
    property bool showServer: true

    Item {
        implicitWidth: form.availableWidth
        implicitHeight: column.implicitHeight + 2 * Theme.gapXl

        ColumnLayout {
            id: column

            anchors.top: parent.top
            anchors.topMargin: Theme.gapXl
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width - 2 * Theme.gapXl, 760)
            spacing: Theme.gapL

            SectionCard {
                Layout.fillWidth: true
                visible: form.showServer
                title: qsTr("Server")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            objectName: "settingsServer"
                            text: qsTr("%1 (%2), signed in as %3").arg(Backend.session.serverName).arg(Backend.session.apiHost).arg(Backend.session.username)
                            color: Theme.text
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("A token belongs to one server. To use another server, sign out and choose it on the sign-in page.")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.WordWrap
                        }
                    }
                    AppButton {
                        objectName: "settingsSignOut"
                        text: qsTr("Sign out")
                        iconName: "logout"
                        enabled: Backend.activity.runningCount === 0
                        onClicked: Backend.session.signOut()
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Archiving")

                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingGuessName"
                    title: qsTr("Suggest a dataset name")
                    description: qsTr("Taken from the folder or file you add first.")
                    checked: form.settings.guessDatasetName
                    onToggled: checked => form.settings.guessDatasetName = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingUsePgroupPath"
                    title: qsTr("Open file dialogs in the pgroup folder")
                    description: qsTr("/das/work/p12/p12345 for p12345, when that folder exists on this computer.")
                    checked: form.settings.usePgroupAsDefaultPath
                    onToggled: checked => form.settings.usePgroupAsDefaultPath = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingInvestigatorIsPI"
                    title: qsTr("Use the principal investigator as investigator")
                    description: qsTr("For derived data. Otherwise your own email address is filled in.")
                    checked: form.settings.investigatorIsPI
                    onToggled: checked => form.settings.investigatorIsPI = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingEnableIngest"
                    title: qsTr("Register datasets in the catalogue by default")
                    description: qsTr("The starting value of the first box on the \"Review and send\" step.")
                    checked: form.settings.enableIngest
                    onToggled: checked => form.settings.enableIngest = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingEnableArchive"
                    title: qsTr("Send files to tape by default")
                    description: qsTr("The starting value of the second box on the \"Review and send\" step.")
                    checked: form.settings.enableArchive
                    onToggled: checked => form.settings.enableArchive = checked
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Proposals")

                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingSaveProposal"
                    title: qsTr("Remember PI details I type in")
                    description: qsTr("For pgroups without a proposal in SciCat. Kept in the log folder on this computer.")
                    checked: form.settings.saveProposalInfo
                    onToggled: checked => form.settings.saveProposalInfo = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingLoadProposal"
                    title: qsTr("Fill in remembered PI details")
                    description: qsTr("When SciCat has no proposal for the pgroup.")
                    checked: form.settings.loadProposalInfo
                    onToggled: checked => form.settings.loadProposalInfo = checked
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Retrieving")

                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingRequestRetrieved"
                    title: qsTr("Allow requesting datasets that were already retrieved")
                    description: qsTr("Useful when the copy on the retrieve server has expired.")
                    checked: form.settings.requestAlreadyRetrieved
                    onToggled: checked => form.settings.requestAlreadyRetrieved = checked
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Files written for scicat-cli")
                subtitle: qsTr("Each ingestion gets its own folder with the metadata, the file listing and the output of scicat-cli.")

                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Log folder")

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapS

                        StoredTextField {
                            Layout.fillWidth: true
                            objectName: "settingLogPath"
                            stored: form.settings.logPath
                            Accessible.name: qsTr("Log folder")
                            onCommitted: text => form.settings.logPath = text
                        }
                        AppButton {
                            text: qsTr("Choose…")
                            onClicked: {
                                logFolderDialog.currentFolder = Backend.folderUrl(form.settings.logPath)
                                logFolderDialog.open()
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        label: qsTr("Name of the metadata file")
                        help: qsTr("Ends in .json")

                        StoredTextField {
                            Layout.fillWidth: true
                            objectName: "settingMetadataFile"
                            stored: form.settings.metadataFileName
                            Accessible.name: qsTr("Name of the metadata file")
                            onCommitted: text => form.settings.metadataFileName = text
                        }
                    }
                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.alignment: Qt.AlignTop
                        label: qsTr("Name of the file listing")
                        help: qsTr("Ends in .txt")

                        StoredTextField {
                            Layout.fillWidth: true
                            objectName: "settingListingFile"
                            stored: form.settings.fileListingFileName
                            Accessible.name: qsTr("Name of the file listing")
                            onCommitted: text => form.settings.fileListingFileName = text
                        }
                    }
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingSaveLog"
                    title: qsTr("Keep the output of scicat-cli as ingestion.log")
                    checked: form.settings.saveLog
                    onToggled: checked => form.settings.saveLog = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingDeleteLog"
                    title: qsTr("Delete the folder when the ingestion ends")
                    description: qsTr("Nothing is kept then, also not ingestion.log.")
                    checked: form.settings.deleteLog
                    onToggled: checked => form.settings.deleteLog = checked
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("scicat-cli")
                subtitle: qsTr("The program that does the ingesting and copying. The one shipped with SciCat is found by itself.")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM

                    KeyValue {
                        Layout.fillWidth: true
                        objectName: "settingCliPath"
                        label: qsTr("Program")
                        value: form.settings.cliPath
                        mono: true
                        labelWidth: 80
                    }
                    StatusTag {
                        objectName: "settingCliState"
                        text: form.settings.cliFound ? qsTr("Found") : qsTr("Not found")
                        tone: form.settings.cliFound ? "success" : "error"
                        iconName: form.settings.cliFound ? "checkCircle" : "error"
                    }
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingShowCommand"
                    title: qsTr("Show the command in the log")
                    description: qsTr("The first line of every log is the scicat-cli call, with the token hidden.")
                    checked: form.settings.showCommand
                    onToggled: checked => form.settings.showCommand = checked
                }
                SwitchRow {
                    Layout.fillWidth: true
                    objectName: "settingDryMode"
                    title: qsTr("Do not run scicat-cli")
                    description: qsTr("For trying the application: the files are written, the program is not started and nothing is sent.")
                    checked: form.settings.disableSystemCalls
                    onToggled: checked => form.settings.disableSystemCalls = checked
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gapM

                Label {
                    Layout.fillWidth: true
                    text: qsTr("SciCat %1 · settings in %2").arg(Backend.version).arg(form.settings.location)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideMiddle
                }
                AppButton {
                    objectName: "settingsReset"
                    text: qsTr("Reset all settings")
                    onClicked: {
                        form.settings.resetToDefaults()
                        form.bringIntoView(resetDone)
                    }
                }
            }
            // Said here, next to the button, on the page and in the dialog of the sign-in page alike. It stays
            // until it is closed or a setting is changed: from then on "Undo" would take that change back too.
            InlineMessage {
                id: resetDone

                Layout.fillWidth: true
                objectName: "settingsResetDone"
                visible: form.settings.canUndoReset
                tone: "success"
                text: qsTr("All settings are back to their defaults.")
                actionText: qsTr("Undo")
                dismissible: true
                onActionTriggered: form.settings.undoReset()
                onDismissed: form.settings.forgetReset()
            }
        }
    }

    FolderDialog {
        id: logFolderDialog

        title: qsTr("Choose the log folder")
        onAccepted: form.settings.logPath = form.settings.toLocalPath(selectedFolder)
    }
}
