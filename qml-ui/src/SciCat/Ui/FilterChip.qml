import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// One choice of a single-choice filter, with the number of entries behind it.
AbstractButton {
    id: control

    property int count: -1

    implicitHeight: 30
    implicitWidth: row.implicitWidth + 24
    // not checkable: which chip is chosen is the owner's to say, a second click must not uncheck it
    hoverEnabled: true
    Accessible.role: Accessible.RadioButton
    Accessible.name: count >= 0 ? qsTr("%1, %2").arg(text).arg(count) : text
    Accessible.checked: checked

    background: Rectangle {
        radius: 15
        color: control.checked ? Theme.selected : control.hovered ? Theme.hover : "transparent"
        border.width: 1
        border.color: control.checked ? Theme.accent : Theme.outlineStrong

        FocusRing {
            shown: control.visualFocus
            radius: 18
        }
    }
    contentItem: Item {
        RowLayout {
            id: row

            anchors.centerIn: parent
            spacing: 6

            Label {
                text: control.text
                color: Theme.text
                font.pixelSize: Theme.fontSmall
                font.weight: control.checked ? Font.DemiBold : Font.Normal
            }
            Label {
                text: control.count
                visible: control.count >= 0
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }
    }
}
