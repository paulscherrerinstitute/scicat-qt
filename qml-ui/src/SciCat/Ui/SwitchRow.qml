import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// A preference that takes effect at once: what it does in words, and the switch.
RowLayout {
    id: root

    property string title
    property string description
    property alias checked: toggle.checked

    signal toggled(bool checked)

    spacing: Theme.gapL

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        // the words belong to the switch: a click on them flips it, as with a labelled check box
        TapHandler {
            onTapped: {
                toggle.toggle()
                root.toggled(toggle.checked)
            }
        }
        Label {
            Layout.fillWidth: true
            text: root.title
            color: Theme.text
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Accessible.ignored: true
        }
        Label {
            Layout.fillWidth: true
            text: root.description
            visible: text !== ""
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
            Accessible.ignored: true
        }
    }
    Switch {
        id: toggle

        Layout.alignment: Qt.AlignVCenter
        padding: 4
        Accessible.name: root.title
        Accessible.description: root.description
        onToggled: root.toggled(checked)

        FocusRing {
            shown: toggle.visualFocus
            inset: 0
        }
    }
}
