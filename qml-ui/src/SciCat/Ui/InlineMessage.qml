import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A message that stays next to what it is about until the user acts on it or closes it.
Rectangle {
    id: root

    property string tone: "info"
    property string title
    property string text
    property string actionText
    property bool dismissible: false

    signal actionTriggered()
    signal dismissed()

    implicitHeight: content.implicitHeight + 2 * Theme.gapM
    implicitWidth: 320
    radius: Theme.radius
    color: Theme.toneFill(tone)
    Accessible.role: tone === "error" || tone === "warning" ? Accessible.AlertMessage : Accessible.Note
    Accessible.name: title !== "" ? title + ". " + text : text

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.margins: Theme.gapM
        spacing: Theme.gapM

        Icon {
            Layout.alignment: Qt.AlignTop
            name: Theme.toneIcon(root.tone)
            size: 20
            color: Theme.toneText(root.tone)
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.title
                visible: text !== ""
                color: Theme.toneText(root.tone)
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                text: root.text
                visible: text !== ""
                color: Theme.toneText(root.tone)
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
            }
        }
        AppButton {
            Layout.alignment: Qt.AlignVCenter
            objectName: "messageAction"
            text: root.actionText
            visible: text !== ""
            onClicked: root.actionTriggered()
        }
        IconButton {
            Layout.alignment: Qt.AlignTop
            objectName: "messageDismiss"
            iconName: "close"
            label: qsTr("Dismiss this message")
            iconColor: Theme.toneText(root.tone)
            visible: root.dismissible
            onClicked: root.dismissed()
        }
    }
}
