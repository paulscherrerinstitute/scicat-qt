import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A labelled input: label above, the control, then help or (after a failed attempt) what is wrong and how to fix it.
ColumnLayout {
    id: root

    property string label
    property string help
    property string problem
    property bool optional: false
    default property alias content: slot.data

    spacing: 4

    Label {
        Layout.fillWidth: true
        text: root.optional ? qsTr("%1 (optional)").arg(root.label) : root.label
        visible: root.label !== ""
        color: Theme.text
        font.pixelSize: Theme.fontBody
        font.weight: Font.Medium
        wrapMode: Text.WordWrap
    }
    ColumnLayout {
        id: slot

        Layout.fillWidth: true
        spacing: 4
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        visible: root.problem !== "" || root.help !== ""

        Icon {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 1
            name: "error"
            size: 14
            color: Theme.toneText("error")
            visible: root.problem !== ""
        }
        Label {
            Layout.fillWidth: true
            objectName: root.problem !== "" ? "fieldProblem" : "fieldHelp"
            text: root.problem !== "" ? root.problem : root.help
            color: root.problem !== "" ? Theme.toneText("error") : Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
            Accessible.role: root.problem !== "" ? Accessible.AlertMessage : Accessible.StaticText
        }
    }
}
