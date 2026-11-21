import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// What a list or page shows instead of nothing: what belongs here and the way to get it.
Item {
    id: root

    property string iconName: "info"
    property string title
    property string text
    property string actionText
    property string actionIcon
    property bool busy: false

    signal actionTriggered()

    implicitHeight: column.implicitHeight + 2 * Theme.gapXl
    implicitWidth: 360

    ColumnLayout {
        id: column

        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.gapXl, 440)
        spacing: Theme.gapS

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            visible: root.busy
            running: root.busy && root.visible
        }
        Icon {
            Layout.alignment: Qt.AlignHCenter
            name: root.iconName
            size: 36
            color: Theme.textSecondary
            visible: !root.busy
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: Theme.gapXs
            text: root.title
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontTitle
            font.weight: Font.Medium
            color: Theme.text
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            text: root.text
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontBody
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        AppButton {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.gapS
            objectName: "emptyAction"
            text: root.actionText
            iconName: root.actionIcon
            variant: AppButton.Filled
            visible: text !== ""
            onClicked: root.actionTriggered()
        }
    }
}
