pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Everything that takes time, in one place: runs started here and the retrieve requests SciCat knows about.
Item {
    id: page

    readonly property ActivityModel activity: Backend.activity
    // The task whose log is open. Tasks come and go and change their place in the list, so it is kept by its ID.
    property string openId

    signal notify(string message)

    // The retrieve requests are asked for when the page is first opened, unless Retrieve has asked already. From
    // then on the list follows the ones that are waiting; "Check SciCat now" asks for all of them again.
    onVisibleChanged: if (visible) Backend.retrieve.ensureJobsLoaded()
    Component.onCompleted: if (visible) Backend.retrieve.ensureJobsLoaded()

    function toneOf(state: int): string {
        switch (state) {
        case ActivityModel.Done: return "success"
        case ActivityModel.Failed: return "error"
        case ActivityModel.Running: return "info"
        case ActivityModel.Cancelled: return "warning"
        }
        return "neutral"
    }

    function iconOf(state: int): string {
        switch (state) {
        case ActivityModel.Done: return "checkCircle"
        case ActivityModel.Failed: return "error"
        case ActivityModel.Running: return "refresh"
        case ActivityModel.Cancelled: return "stop"
        }
        return "hourglass"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.gapXl
        spacing: Theme.gapM

        PageHeader {
            Layout.fillWidth: true
            title: page.activity.runningCount === 0 ? qsTr("Nothing is running")
                : page.activity.runningCount === 1 ? qsTr("1 task is running") : qsTr("%1 tasks are running").arg(page.activity.runningCount)
            subtitle: qsTr("Ingestions and copies started in this session, then your retrieve requests in SciCat. The list updates by itself.")

            AppButton {
                objectName: "activityClear"
                text: qsTr("Clear finished")
                variant: AppButton.Plain
                enabled: page.activity.finishedLocalCount > 0
                onClicked: page.activity.clearFinished()
            }
            AppButton {
                objectName: "activityRefresh"
                text: qsTr("Check SciCat now")
                iconName: "refresh"
                // only the retrieve requests are asked for again; they do not depend on the pgroup
                onClicked: Backend.retrieve.refreshJobs()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.outline

            EmptyState {
                anchors.fill: parent
                objectName: "activityEmpty"
                visible: page.activity.count === 0
                iconName: "activity"
                title: qsTr("Nothing here yet")
                text: qsTr("When you archive a dataset, request one from tape or copy one to disk, you can follow it here.")
            }

            ListView {
                id: list

                anchors.fill: parent
                anchors.margins: 1
                objectName: "activityList"
                visible: page.activity.count > 0
                clip: true
                model: page.activity
                activeFocusOnTab: true
                Accessible.role: Accessible.List
                Accessible.name: qsTr("Activity")
                ScrollBar.vertical: ScrollBar {}

                section.property: "local"
                section.delegate: Rectangle {
                    required property string section

                    width: ListView.view.width
                    height: 34
                    color: Theme.surfaceAlt

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        x: Theme.gapL
                        text: parent.section === "true" ? qsTr("Started in this session") : qsTr("Retrieve requests in SciCat")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                        Accessible.role: Accessible.Heading
                    }
                }

                delegate: Item {
                    id: row

                    required property int index
                    required property string taskId
                    required property int kind
                    required property string title
                    required property string detail
                    required property int taskState
                    required property string stateText
                    required property string timeText
                    required property string log
                    required property bool canCancel
                    required property string folder
                    required property bool local

                    readonly property bool open: page.openId !== "" && page.openId === taskId && log !== ""

                    width: ListView.view.width
                    height: open ? 300 : 60
                    objectName: "activityRow"
                    Accessible.role: Accessible.ListItem
                    Accessible.name: qsTr("%1, %2, %3").arg(title).arg(stateText).arg(timeText)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.gapL
                        anchors.rightMargin: Theme.gapM
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 59
                            spacing: Theme.gapM

                            Icon {
                                name: row.kind === ActivityModel.Ingest ? "upload" : row.kind === ActivityModel.Copy ? "download" : "archive"
                                size: 20
                                color: Theme.textSecondary
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    objectName: "activityRowTitle"
                                    text: row.title
                                    color: Theme.text
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    objectName: "activityRowDetail"
                                    text: row.detail !== "" ? qsTr("%1 · %2").arg(row.timeText).arg(row.detail) : row.timeText
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideRight
                                }
                            }
                            StatusTag {
                                objectName: "activityRowState"
                                text: row.stateText
                                tone: page.toneOf(row.taskState)
                                iconName: page.iconOf(row.taskState)
                            }
                            AppButton {
                                objectName: "activityRowLog"
                                text: row.open ? qsTr("Hide log") : qsTr("Show log")
                                variant: AppButton.Plain
                                visible: row.log !== ""
                                onClicked: page.openId = row.open ? "" : row.taskId
                            }
                            AppButton {
                                objectName: "activityRowFolder"
                                text: qsTr("Show folder")
                                variant: AppButton.Plain
                                visible: row.folder !== "" && !row.canCancel
                                onClicked: Backend.showFolder(row.folder)
                            }
                            AppButton {
                                objectName: "activityRowStop"
                                text: qsTr("Stop")
                                iconName: "stop"
                                visible: row.canCancel
                                onClicked: page.activity.cancel(row.index)
                            }
                        }
                        // made only for the row whose log is open
                        Loader {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.bottomMargin: Theme.gapM
                            active: row.open
                            visible: row.open
                            sourceComponent: LogView {
                                objectName: "activityRowOutput"
                                text: row.log
                                label: qsTr("Output of %1").arg(row.title)
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Theme.outline
                        }
                    }
                }
            }
            FocusRing {
                shown: list.activeFocus
                inset: 0
                radius: Theme.radius
            }
        }
    }
}
