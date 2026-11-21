import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// A column title that sorts the list. The arrow and the announced name say which column sorts and in which direction.
AbstractButton {
    id: control

    property bool active: false
    property bool descending: false
    property bool alignRight: false

    implicitHeight: 32
    hoverEnabled: true
    Accessible.role: Accessible.ColumnHeader
    Accessible.name: !active ? qsTr("%1, sort by this column").arg(text)
        : descending ? qsTr("%1, sorted descending").arg(text) : qsTr("%1, sorted ascending").arg(text)

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.hovered || control.down ? Theme.hover : "transparent"

        FocusRing {
            shown: control.visualFocus
            inset: 0
        }
    }
    contentItem: RowLayout {
        spacing: 2
        layoutDirection: control.alignRight ? Qt.RightToLeft : Qt.LeftToRight

        Label {
            Layout.leftMargin: control.alignRight ? 0 : 6
            Layout.rightMargin: control.alignRight ? 6 : 0
            text: control.text
            color: control.active ? Theme.text : Theme.textSecondary
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Icon {
            name: control.descending ? "arrowDown" : "arrowUp"
            size: 13
            color: Theme.text
            visible: control.active
        }
        Item {
            Layout.fillWidth: true
        }
    }
}
