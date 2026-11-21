import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A read-only fact: what it is, then its value as text that can be selected and copied.
RowLayout {
    id: root

    property string label
    property string value
    property bool mono: false
    property int labelWidth: 120

    spacing: Theme.gapM
    Accessible.role: Accessible.StaticText
    Accessible.name: label + ": " + (value !== "" ? value : qsTr("not set"))

    Label {
        Layout.preferredWidth: root.labelWidth
        Layout.alignment: Qt.AlignTop
        text: root.label
        color: Theme.textSecondary
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
        Accessible.ignored: true
    }
    TextEdit {
        Layout.fillWidth: true
        text: root.value !== "" ? root.value : "–"
        color: root.value !== "" ? Theme.text : Theme.textSecondary
        font.pixelSize: root.mono ? Theme.fontSmall : Theme.fontBody
        font.family: root.mono ? Theme.monoFamily : Application.font.family
        readOnly: true
        selectByMouse: true
        // at a word boundary where there is one; a long path or ID still breaks anywhere
        wrapMode: TextEdit.Wrap
        selectionColor: Theme.accent
        selectedTextColor: Theme.textOnAccent
        Accessible.ignored: true
    }
}
