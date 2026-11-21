pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Step 3: what will be sent where, anything still missing, and what should happen.
ScrollPage {
    id: step

    readonly property ArchiveDraft draft: Backend.archive
    readonly property var stepNames: [qsTr("Files"), qsTr("Details")]

    signal stepRequested(int step)

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

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: problemColumn.implicitHeight + 2 * Theme.gapM
                objectName: "reviewProblems"
                visible: step.draft.problems.length > 0
                radius: Theme.radius
                color: Theme.toneFill("error")
                Accessible.role: Accessible.AlertMessage
                Accessible.name: qsTr("Not ready to send yet")

                ColumnLayout {
                    id: problemColumn

                    anchors.fill: parent
                    anchors.margins: Theme.gapM
                    spacing: Theme.gapXs

                    Label {
                        text: qsTr("Not ready to send yet")
                        color: Theme.toneText("error")
                        font.weight: Font.DemiBold
                    }
                    Repeater {
                        model: step.draft.problems

                        RowLayout {
                            id: problemRow

                            required property var modelData

                            Layout.fillWidth: true
                            spacing: Theme.gapS

                            Label {
                                Layout.fillWidth: true
                                text: problemRow.modelData.text
                                color: Theme.toneText("error")
                                wrapMode: Text.WordWrap
                            }
                            AppButton {
                                objectName: "reviewProblemLink"
                                text: problemRow.modelData.step < 0 ? qsTr("Go to Overview") : qsTr("Go to %1").arg(step.stepNames[problemRow.modelData.step])
                                onClicked: step.stepRequested(problemRow.modelData.step)
                            }
                        }
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                objectName: "reviewSummary"
                title: qsTr("What will be sent")

                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    label: qsTr("Dataset")
                    value: step.draft.datasetName
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    label: qsTr("Kind")
                    value: step.draft.raw ? qsTr("Raw data from %1").arg(step.draft.creationLocation !== "" ? step.draft.creationLocation : qsTr("an instrument not chosen yet"))
                                          : qsTr("Derived data by %1").arg(step.draft.investigator)
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    label: qsTr("Files")
                    value: step.draft.files.summary
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    label: qsTr("Source folder")
                    value: step.draft.files.sourceFolder
                    mono: true
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    label: qsTr("Owner")
                    value: step.draft.ownerEmail !== "" ? qsTr("%1 (%2), group %3").arg(step.draft.owner.trim()).arg(step.draft.ownerEmail).arg(step.draft.ownerGroup)
                                                        : qsTr("%1, group %2").arg(step.draft.owner.trim()).arg(step.draft.ownerGroup)
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    visible: step.draft.raw
                    label: qsTr("Principal investigator")
                    value: step.draft.principalInvestigator
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    visible: step.draft.proposalId !== ""
                    label: qsTr("Proposal ID")
                    value: step.draft.proposalId
                    mono: true
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    visible: step.draft.raw
                    label: qsTr("Scientific metadata")
                    value: step.draft.metadata.fieldCount === 1 ? qsTr("1 entry") : qsTr("%1 entries").arg(step.draft.metadata.fieldCount)
                }
                KeyValue {
                    Layout.fillWidth: true
                    labelWidth: 160
                    visible: !step.draft.raw
                    label: qsTr("Input datasets")
                    value: step.draft.inputDatasets.count === 0 ? qsTr("none") : step.draft.inputDatasets.count
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM

                    Label {
                        Layout.preferredWidth: 160
                        text: qsTr("Goes to")
                        color: Theme.textSecondary
                    }
                    Label {
                        text: Backend.session.apiHost
                        color: Theme.text
                    }
                    StatusTag {
                        objectName: "reviewEnvironment"
                        text: Backend.session.environment === Session.Production ? qsTr("Production")
                            : Backend.session.environment === Session.LocalTest ? qsTr("Local test") : qsTr("%1: test data").arg(Backend.session.environmentLabel)
                        tone: Backend.session.environment === Session.Production ? "neutral"
                            : Backend.session.environment === Session.QA ? "qa" : Backend.session.environment === Session.Development ? "development" : "local"
                        strong: true
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("What should happen")

                CheckBox {
                    id: ingestBox

                    objectName: "ingestOption"
                    text: qsTr("Register the dataset in the catalogue")
                    checked: step.draft.ingest
                    onToggled: step.draft.ingest = checked
                }
                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 36
                    Layout.topMargin: -Theme.gapL
                    text: step.draft.ingest ? qsTr("scicat-cli checks the files and metadata and creates the dataset in SciCat.")
                                            : qsTr("Not ticked: a dry run. scicat-cli only checks the files and metadata, and nothing is registered.")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                CheckBox {
                    objectName: "archiveOption"
                    text: qsTr("Send the files to the tape archive right away")
                    checked: step.draft.autoArchive && step.draft.ingest
                    enabled: step.draft.ingest
                    onToggled: step.draft.autoArchive = checked
                }
                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 36
                    Layout.topMargin: -Theme.gapL
                    text: !step.draft.ingest ? qsTr("Only a registered dataset can be archived.")
                        : step.draft.autoArchive ? qsTr("The archive job is requested as soon as the dataset is registered.")
                                                 : qsTr("Not ticked: the dataset is registered only. You can archive it later from the SciCat website.")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
