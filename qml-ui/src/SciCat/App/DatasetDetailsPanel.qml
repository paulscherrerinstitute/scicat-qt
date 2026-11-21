import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// The dataset the user is looking at: its facts, its files and its scientific metadata.
Rectangle {
    id: panel

    required property DatasetDetails details
    property string stateText
    property string stateTone
    property string stateIcon

    // the four facts give way first when the panel is short: size, date and kind are in the table as well
    readonly property bool roomy: height >= 470

    signal notify(string message)

    radius: Theme.radius
    clip: true
    color: Theme.surface
    border.width: 1
    border.color: Theme.outline

    EmptyState {
        anchors.fill: parent
        objectName: "detailsEmpty"
        visible: !panel.details.available
        iconName: "list"
        title: qsTr("No dataset chosen")
        text: qsTr("Click a dataset to see its files and scientific metadata here.")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapL
        spacing: Theme.gapM
        visible: panel.details.available

        // the name has the full width and may take a second line; its state follows on a line of its own
        Label {
            Layout.fillWidth: true
            objectName: "detailsName"
            text: panel.details.name
            color: Theme.text
            font.pixelSize: Theme.fontTitle
            font.weight: Font.Medium
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            Accessible.role: Accessible.Heading
        }
        StatusTag {
            Layout.topMargin: -Theme.gapXs
            objectName: "detailsState"
            text: panel.stateText
            tone: panel.stateTone
            iconName: panel.stateIcon
        }
        Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: panel.details.jobNote
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapXs

            Label {
                Layout.fillWidth: true
                objectName: "detailsPid"
                text: panel.details.pid
                color: Theme.textSecondary
                font.family: Theme.monoFamily
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideMiddle
            }
            IconButton {
                iconName: "copy"
                label: qsTr("Copy the dataset ID")
                onClicked: {
                    Backend.copyToClipboard(panel.details.pid)
                    panel.notify(qsTr("Dataset ID copied."))
                }
            }
        }
        KeyValue {
            Layout.fillWidth: true
            visible: panel.roomy
            label: qsTr("Size")
            value: panel.details.sizeText
            labelWidth: 96
        }
        KeyValue {
            Layout.fillWidth: true
            visible: panel.roomy
            label: qsTr("Created")
            value: panel.details.createdText
            labelWidth: 96
        }
        KeyValue {
            Layout.fillWidth: true
            visible: panel.roomy
            label: qsTr("Kind")
            value: panel.details.type === "raw" ? qsTr("Raw data") : panel.details.type === "derived" ? qsTr("Derived data") : panel.details.type
            labelWidth: 96
        }
        KeyValue {
            Layout.fillWidth: true
            visible: panel.roomy
            label: qsTr("Source folder")
            value: panel.details.sourceFolder
            mono: true
            labelWidth: 96
        }

        TabBar {
            id: tabs

            Layout.fillWidth: true
            objectName: "detailsTabs"

            TabButton {
                objectName: "detailsFilesTab"
                text: qsTr("Files")
            }
            TabButton {
                objectName: "detailsMetadataTab"
                // short enough for the narrowest panel; the list under it is named in full
                text: qsTr("Metadata")
                Accessible.name: qsTr("Scientific metadata")
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            Item {
                // a message with a button can be taller than a short panel
                clip: true

                SkeletonRows {
                    anchors.fill: parent
                    visible: panel.details.filesState === DatasetDetails.Loading
                    rows: 6
                    rowHeight: 34
                    clip: true
                }
                EmptyState {
                    anchors.fill: parent
                    objectName: "detailsFilesFailed"
                    visible: panel.details.filesState === DatasetDetails.Failed
                    iconName: "error"
                    title: panel.details.filesNote
                    actionText: qsTr("Try again")
                    onActionTriggered: panel.details.retry()
                }
                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.gapXs
                    visible: panel.details.filesState === DatasetDetails.Loaded

                    Label {
                        Layout.fillWidth: true
                        objectName: "detailsFilesNote"
                        text: panel.details.filesNote
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ListView {
                            id: fileList

                            anchors.fill: parent
                            objectName: "detailsFileList"
                            clip: true
                            model: panel.details.files
                            activeFocusOnTab: true
                            Accessible.role: Accessible.List
                            Accessible.name: qsTr("Files of %1").arg(panel.details.name)
                            ScrollBar.vertical: ScrollBar {}

                            delegate: RowLayout {
                                id: fileRow

                                required property string path
                                required property string sizeText
                                required property string timeText

                                width: ListView.view.width - 10
                                height: 34
                                spacing: Theme.gapS
                                objectName: "detailsFileRow"
                                Accessible.role: Accessible.ListItem
                                Accessible.name: qsTr("%1, %2").arg(path).arg(sizeText)

                                Label {
                                    Layout.fillWidth: true
                                    text: fileRow.path
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideMiddle
                                    ToolTip.text: fileRow.timeText !== "" ? qsTr("%1, changed %2").arg(fileRow.path).arg(fileRow.timeText) : fileRow.path
                                    ToolTip.visible: fileHover.hovered
                                    ToolTip.delay: 600

                                    HoverHandler {
                                        id: fileHover
                                    }
                                }
                                Label {
                                    objectName: "detailsFileSize"
                                    text: fileRow.sizeText
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                        }
                        FocusRing {
                            shown: fileList.activeFocus
                            inset: 0
                        }
                    }
                }
            }

            ScrollView {
                id: metadataScroll

                contentWidth: availableWidth

                ColumnLayout {
                    width: metadataScroll.availableWidth
                    spacing: Theme.gapS

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapS
                        visible: panel.details.metadataState === DatasetDetails.Loading

                        BusyIndicator {
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            running: visible && panel.details.metadataState === DatasetDetails.Loading
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Loading the scientific metadata…")
                            color: Theme.textSecondary
                            wrapMode: Text.WordWrap
                        }
                    }
                    InlineMessage {
                        Layout.fillWidth: true
                        visible: panel.details.metadataState === DatasetDetails.Failed
                        tone: "error"
                        text: panel.details.metadataNote
                        actionText: qsTr("Try again")
                        onActionTriggered: panel.details.retry()
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: panel.details.metadataState === DatasetDetails.Loaded && panel.details.metadataNote !== ""
                        text: panel.details.metadataNote
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                    MetadataEditor {
                        Layout.fillWidth: true
                        objectName: "detailsMetadata"
                        visible: panel.details.metadataState === DatasetDetails.Loaded && panel.details.metadataNote === ""
                        metadata: panel.details.metadata
                        editable: false
                    }
                }
            }
        }
    }
}
