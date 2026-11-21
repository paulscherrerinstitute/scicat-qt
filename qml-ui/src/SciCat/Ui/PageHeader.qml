import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// What a page is for, said once, with room for the page's own actions on the right.
RowLayout {
    id: root

    property string title
    property string subtitle
    default property alias actions: slot.data

    spacing: Theme.gapL

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            Layout.fillWidth: true
            text: root.title
            color: Theme.text
            font.pixelSize: Theme.fontHeading
            font.weight: Font.Medium
            elide: Text.ElideRight
            Accessible.role: Accessible.Heading
        }
        Label {
            Layout.fillWidth: true
            text: root.subtitle
            visible: text !== ""
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
    RowLayout {
        id: slot

        Layout.alignment: Qt.AlignVCenter
        spacing: Theme.gapS
    }
}
