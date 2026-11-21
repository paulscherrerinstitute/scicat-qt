pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Step 1: which files and folders make up the dataset.
Item {
    id: step

    readonly property ArchiveDraft draft: Backend.archive
    readonly property FileEntryModel files: Backend.archive.files
    property list<string> removedPaths

    signal notify(string message)
    // a change that can be taken back: the window shows the message with an "Undo" that calls undo()
    signal undoOffered(string message, var undo)

    function added(count: int, offered: int) {
        // worded so that it reads right for one as for many
        if (count < offered)
            step.notify(count === 0 ? qsTr("Already in the list.") : qsTr("Added: %1. Already in the list: %2.").arg(count).arg(offered - count))
    }

    // where the file choosers start is asked each time: it follows the pgroup and a preference
    function chooseFolder() {
        folderDialog.currentFolder = draft.pickerStartFolder()
        folderDialog.open()
    }

    function chooseFiles() {
        fileDialog.currentFolder = draft.pickerStartFolder()
        fileDialog.open()
    }

    DropArea {
        id: dropArea

        anchors.fill: parent
        objectName: "fileDropArea"
        onDropped: drop => {
            if (drop.hasUrls) {
                step.added(step.files.addUrls(drop.urls), drop.urls.length)
                drop.accept()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapXl
        spacing: Theme.gapM

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("What belongs to this dataset?")
            subtitle: qsTr("Folders are archived with everything in them. You can also drop files and folders here.")

            AppButton {
                objectName: "addFolder"
                text: qsTr("Add folder…")
                iconName: "folder"
                variant: step.files.count === 0 ? AppButton.Filled : AppButton.Tonal
                onClicked: step.chooseFolder()
            }
            AppButton {
                objectName: "addFiles"
                text: qsTr("Add files…")
                iconName: "file"
                onClicked: step.chooseFiles()
            }
        }

        InlineMessage {
            Layout.fillWidth: true
            objectName: "filesProblem"
            visible: step.draft.filesProblem !== ""
            tone: "error"
            text: step.draft.filesProblem
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.width: dropArea.containsDrag ? 2 : 1
            border.color: dropArea.containsDrag ? Theme.accent : Theme.outline

            EmptyState {
                anchors.fill: parent
                objectName: "filesEmpty"
                visible: step.files.count === 0
                iconName: "folder"
                title: dropArea.containsDrag ? qsTr("Drop to add") : qsTr("Nothing chosen yet")
                text: qsTr("Add the folders or files that make up one dataset, or drop them here.")
            }

            ListView {
                id: list

                anchors.fill: parent
                anchors.margins: 1
                objectName: "fileList"
                visible: step.files.count > 0
                clip: true
                model: step.files
                activeFocusOnTab: true
                Accessible.role: Accessible.List
                Accessible.name: qsTr("Files and folders of the dataset")
                ScrollBar.vertical: ScrollBar {}

                delegate: Item {
                    id: row

                    required property int index
                    required property string name
                    required property string folder
                    required property string detail
                    required property bool isFolder
                    required property bool exists
                    required property bool scanning

                    width: ListView.view.width
                    height: 52
                    objectName: "fileRow"
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("%1 in %2, %3").arg(name).arg(folder).arg(detail)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.gapL
                        anchors.rightMargin: Theme.gapS
                        spacing: Theme.gapM

                        Icon {
                            name: row.isFolder ? "folder" : "file"
                            size: 20
                            color: row.exists ? Theme.textSecondary : Theme.toneText("error")
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: row.name
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.Medium
                                elide: Text.ElideMiddle
                            }
                            Label {
                                Layout.fillWidth: true
                                text: row.folder
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSmall
                                elide: Text.ElideMiddle
                            }
                        }
                        Label {
                            objectName: "fileRowDetail"
                            text: row.detail
                            color: row.exists ? Theme.textSecondary : Theme.toneText("error")
                            font.pixelSize: Theme.fontSmall
                        }
                        IconButton {
                            objectName: "fileRowRemove"
                            iconName: "close"
                            label: qsTr("Remove %1 from the list").arg(row.name)
                            onClicked: step.files.removeAt(row.index)
                        }
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.outline
                    }
                }
            }
            FocusRing {
                shown: list.activeFocus
                inset: 0
                radius: Theme.radius
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapM
            visible: step.files.count > 0

            KeyValue {
                Layout.fillWidth: true
                objectName: "sourceFolder"
                label: qsTr("Source folder")
                value: step.files.sourceFolder
                mono: true
                labelWidth: 96
            }
            AppButton {
                objectName: "removeAllFiles"
                text: qsTr("Remove all")
                variant: AppButton.Plain
                onClicked: {
                    step.removedPaths = step.files.takeAll()
                    step.undoOffered(step.removedPaths.length === 1 ? qsTr("Removed 1 entry from the list.") : qsTr("Removed %1 entries from the list.").arg(step.removedPaths.length),
                                     () => step.files.addPaths(step.removedPaths))
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog

        title: qsTr("Add a folder to the dataset")
        onAccepted: step.added(step.files.addUrls([selectedFolder]), 1)
    }
    FileDialog {
        id: fileDialog

        title: qsTr("Add files to the dataset")
        fileMode: FileDialog.OpenFiles
        onAccepted: step.added(step.files.addUrls(selectedFiles), selectedFiles.length)
    }
}
