import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Archiving a dataset in three steps, then the run of scicat-cli. Nothing typed here is lost when a run fails.
Item {
    id: page

    readonly property ArchiveDraft draft: Backend.archive
    // the result view stays until the user goes back to the form or starts a new dataset
    property bool showRun: draft.run !== null

    signal overviewRequested()
    signal retrieveRequested()
    signal notify(string message)
    // a change that can be taken back: shown as a brief message with an "Undo" that calls undo()
    signal undoOffered(string message, var undo)

    function goTo(step: int) {
        // what is still being typed counts: a button does not take the focus from a field on every platform
        detailsStep.commitPending()
        // going forward passes every step in between, so nothing is skipped unchecked
        for (let s = draft.step; s < step; ++s) {
            if (!draft.checkStep(s)) {
                draft.step = s
                if (s === ArchiveDraft.DetailsStep)
                    detailsStep.focusFirstProblem()
                return
            }
        }
        draft.step = step
    }

    function submit() {
        if (draft.submit()) {
            showRun = true
            return
        }
        // refused: the review step says what is missing, also when this was "Try again" after a failed run
        showRun = false
        draft.step = ArchiveDraft.ReviewStep
    }

    Connections {
        target: page.draft

        function onWarning(text: string) {
            page.notify(text)
        }
        function onStepChanged() {
            // a step is read from its top, also the second time round
            detailsStep.scrollToTop()
            reviewStep.scrollToTop()
        }
    }

    BlockedPage {
        anchors.fill: parent
        visible: !Backend.workspace.groupReady
        onOverviewRequested: page.overviewRequested()
    }

    ArchiveRunView {
        anchors.fill: parent
        visible: Backend.workspace.groupReady && page.showRun && page.draft.run !== null
        run: page.draft.run
        onEditRequested: {
            page.showRun = false
            page.draft.step = ArchiveDraft.DetailsStep
        }
        onRetryRequested: page.submit()
        onNewRequested: keepDetails => {
            page.draft.startNew(keepDetails)
            page.showRun = false
        }
        onRetrieveRequested: page.retrieveRequested()
        onNotify: message => page.notify(message)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: Backend.workspace.groupReady && !(page.showRun && page.draft.run !== null)

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            Stepper {
                anchors.fill: parent
                anchors.leftMargin: Theme.gapXl
                anchors.rightMargin: Theme.gapXl
                objectName: "archiveStepper"
                current: page.draft.step
                titles: [qsTr("Files"), qsTr("Details"), qsTr("Review and send")]
                done: [page.draft.files.count > 0 && page.draft.filesProblem === "", page.draft.step > ArchiveDraft.DetailsStep, false]
                problems: [page.draft.filesProblem !== "",
                           page.draft.datasetNameProblem !== "" || page.draft.instrumentProblem !== "" || page.draft.investigatorProblem !== ""
                           || page.draft.ownerProblem !== "" || page.draft.ownerEmailProblem !== "" || page.draft.ownerGroupProblem !== ""
                           || page.draft.metadataProblem !== "",
                           false]
                onStepRequested: index => page.goTo(index)
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.outline
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: page.draft.step

            ArchiveFilesStep {
                onNotify: message => page.notify(message)
                onUndoOffered: (message, undo) => page.undoOffered(message, undo)
            }
            ArchiveDetailsStep {
                id: detailsStep

                onNotify: message => page.notify(message)
                onUndoOffered: (message, undo) => page.undoOffered(message, undo)
            }
            ArchiveReviewStep {
                id: reviewStep

                onStepRequested: step => {
                    if (step < 0)
                        page.overviewRequested()
                    else
                        page.draft.step = step
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Theme.surface

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.outline
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.gapXl
                anchors.rightMargin: Theme.gapXl
                spacing: Theme.gapM

                AppButton {
                    objectName: "archiveBack"
                    text: qsTr("Back")
                    iconName: "chevronLeft"
                    variant: AppButton.Plain
                    visible: page.draft.step > ArchiveDraft.FilesStep
                    onClicked: {
                        detailsStep.commitPending()
                        page.draft.step = page.draft.step - 1
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: page.draft.files.summary
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                }
                AppButton {
                    objectName: "archiveNext"
                    visible: page.draft.step < ArchiveDraft.ReviewStep
                    text: page.draft.step === ArchiveDraft.FilesStep ? qsTr("Next: details") : qsTr("Next: review")
                    variant: AppButton.Filled
                    onClicked: page.goTo(page.draft.step + 1)
                }
                AppButton {
                    objectName: "archiveSubmit"
                    visible: page.draft.step === ArchiveDraft.ReviewStep
                    text: !page.draft.ingest ? qsTr("Run the check") : page.draft.autoArchive ? qsTr("Register and archive dataset") : qsTr("Register dataset")
                    iconName: "upload"
                    variant: AppButton.Filled
                    onClicked: page.submit()
                }
            }
        }
    }
}
