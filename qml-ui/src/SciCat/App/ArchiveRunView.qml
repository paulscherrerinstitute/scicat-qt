import QtQuick
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// A run of scicat-cli: what it is doing, its output as it comes, and what to do next when it ends.
Item {
    id: view

    required property IngestRun run
    readonly property int runState: run ? run.state : IngestRun.Preparing
    readonly property bool active: run !== null && run.active

    signal editRequested()
    signal retryRequested()
    signal newRequested(bool keepDetails)
    signal retrieveRequested()
    signal notify(string message)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapXl
        spacing: Theme.gapM

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapM
            visible: view.active

            BusyIndicator {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                running: view.active && view.visible
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    objectName: "runTitle"
                    text: !view.run ? "" : view.run.dryRun ? qsTr("Checking %1…").arg(view.run.datasetName) : qsTr("Sending %1 to %2…").arg(view.run.datasetName).arg(view.run.serverLabel)
                    color: Theme.text
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: !view.run ? "" : view.runState === IngestRun.Preparing ? qsTr("Counting the files and writing the input for scicat-cli.")
                        : qsTr("scicat-cli has been running for %1. You can use the other pages meanwhile.").arg(Backend.formatDuration(view.run.elapsedSeconds))
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
            AppButton {
                objectName: "runStop"
                text: qsTr("Stop")
                iconName: "stop"
                onClicked: stopDialog.open()
            }
        }

        InlineMessage {
            Layout.fillWidth: true
            objectName: "runResult"
            visible: view.run !== null && !view.active
            tone: view.runState === IngestRun.Succeeded ? "success" : view.runState === IngestRun.Failed ? "error" : "warning"
            title: !view.run ? "" : view.runState === IngestRun.Succeeded ? (view.run.dryRun ? qsTr("%1 passed the check").arg(view.run.datasetName) : qsTr("%1 is in the catalogue").arg(view.run.datasetName))
                : view.runState === IngestRun.Failed ? qsTr("%1 was not sent").arg(view.run.datasetName) : qsTr("The run was stopped")
            text: view.run ? view.run.resultText : ""
        }

        LogView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            objectName: "runLog"
            text: view.run ? view.run.log : ""
            label: qsTr("Output of scicat-cli")
        }

        GridLayout {
            id: actions

            // what to do next on the right of what to do with the log, or under it when the window is narrow
            readonly property bool sideBySide: aboutLog.implicitWidth + nextSteps.implicitWidth + columnSpacing <= width

            Layout.fillWidth: true
            columns: sideBySide ? 2 : 1
            columnSpacing: Theme.gapL
            rowSpacing: Theme.gapS
            visible: view.run !== null && !view.active

            RowLayout {
                id: aboutLog

                Layout.fillWidth: true
                spacing: Theme.gapS

                AppButton {
                    objectName: "runSaveLog"
                    text: qsTr("Save the log…")
                    variant: AppButton.Plain
                    onClicked: saveDialog.open()
                }
                AppButton {
                    objectName: "runShowFolder"
                    text: qsTr("Show the files given to scicat-cli")
                    variant: AppButton.Plain
                    visible: view.run !== null && view.run.logFolder !== ""
                    onClicked: Backend.showFolder(view.run.logFolder)
                }
                Item {
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                id: nextSteps

                Layout.alignment: Qt.AlignRight
                spacing: Theme.gapS

                AppButton {
                    objectName: "runRetry"
                    text: qsTr("Try again")
                    iconName: "refresh"
                    visible: view.runState === IngestRun.Failed || view.runState === IngestRun.Cancelled
                    onClicked: view.retryRequested()
                }
                AppButton {
                    objectName: "runEdit"
                    text: qsTr("Back to the details")
                    iconName: "chevronLeft"
                    variant: AppButton.Filled
                    visible: view.runState === IngestRun.Failed || view.runState === IngestRun.Cancelled
                    onClicked: view.editRequested()
                }
                AppButton {
                    objectName: "runEditAfterCheck"
                    text: qsTr("Back to the form")
                    iconName: "chevronLeft"
                    variant: AppButton.Filled
                    visible: view.runState === IngestRun.Succeeded && view.run !== null && view.run.dryRun
                    onClicked: view.editRequested()
                }
                AppButton {
                    objectName: "runNewSame"
                    text: qsTr("Another dataset with these details")
                    visible: view.runState === IngestRun.Succeeded && view.run !== null && !view.run.dryRun
                    onClicked: view.newRequested(true)
                }
                AppButton {
                    objectName: "runNew"
                    text: qsTr("Archive another dataset")
                    iconName: "plus"
                    variant: AppButton.Filled
                    visible: view.runState === IngestRun.Succeeded && view.run !== null && !view.run.dryRun
                    onClicked: view.newRequested(false)
                }
            }
        }
    }

    FileDialog {
        id: saveDialog

        title: qsTr("Save the log")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "log"
        nameFilters: [qsTr("Log files (*.log)")]
        onAccepted: {
            const problem = view.run.saveLogTo(selectedFile)
            view.notify(problem !== "" ? problem : qsTr("Log saved."))
        }
    }

    AppDialog {
        id: stopDialog

        objectName: "stopDialog"
        width: Math.min(460, parent.width - 2 * Theme.gapXl)
        title: !view.run ? "" : view.run.dryRun ? qsTr("Stop checking %1?").arg(view.run.datasetName) : qsTr("Stop sending %1?").arg(view.run.datasetName)
        initialFocusItem: stopKeep
        // a run that has ended by itself needs no answer any more
        relevant: view.active

        Label {
            width: parent.width
            text: qsTr("scicat-cli is interrupted. If it has already registered the dataset, the dataset stays in the catalogue. Everything you entered stays in the form.")
            wrapMode: Text.WordWrap
            color: Theme.text
        }
        footer: RowLayout {
            spacing: Theme.gapS

            Item {
                Layout.fillWidth: true
            }
            AppButton {
                Layout.topMargin: Theme.gapM
                Layout.bottomMargin: Theme.gapM
                objectName: "stopConfirm"
                text: qsTr("Stop the run")
                variant: AppButton.Danger
                onClicked: {
                    stopDialog.close()
                    view.run.cancel()
                }
            }
            AppButton {
                id: stopKeep

                Layout.margins: Theme.gapM
                objectName: "stopKeep"
                text: qsTr("Keep running")
                variant: AppButton.Filled
                onClicked: stopDialog.close()
            }
        }
    }
}
