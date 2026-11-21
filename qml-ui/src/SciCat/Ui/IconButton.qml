import QtQuick
import QtQuick.Controls.Material

// A button that shows only an icon. "label" is what a screen reader and the tooltip say.
AbstractButton {
    id: control

    required property string iconName
    required property string label
    property color iconColor: Theme.textSecondary
    property real iconSize: 18

    implicitWidth: 32
    implicitHeight: 32
    hoverEnabled: true
    Accessible.role: Accessible.Button
    Accessible.name: label
    ToolTip.text: label
    ToolTip.visible: hovered || visualFocus
    ToolTip.delay: 500

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.down ? Theme.selected : control.hovered ? Theme.hover : "transparent"

        FocusRing {
            shown: control.visualFocus
            inset: -1
        }
    }
    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            color: control.enabled ? control.iconColor : Theme.textDisabled
        }
    }
}
