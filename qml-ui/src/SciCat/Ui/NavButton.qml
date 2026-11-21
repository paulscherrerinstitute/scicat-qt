import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// One destination in the side navigation: icon and word, a marker on the current one, a count when something runs.
AbstractButton {
    id: control

    required property string iconName
    property bool current: false
    property int badge: 0

    implicitHeight: 40
    implicitWidth: 200
    hoverEnabled: true
    Accessible.role: Accessible.PageTab
    Accessible.name: badge > 0 ? qsTr("%1, %2 running").arg(text).arg(badge) : text
    Accessible.selected: current

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.current ? Theme.brandRaised : control.hovered || control.down ? Theme.brandHover : "transparent"

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: 22
            radius: 1.5
            color: Theme.brandMarker
            visible: control.current
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: Theme.radiusSmall + 2
            color: "transparent"
            border.width: 2
            border.color: Theme.brandMarker
            visible: control.visualFocus
        }
    }
    contentItem: RowLayout {
        spacing: Theme.gapM

        Icon {
            Layout.leftMargin: Theme.gapM
            name: control.iconName
            size: 20
            color: control.current ? Theme.brandText : Theme.brandMuted
        }
        Label {
            Layout.fillWidth: true
            text: control.text
            color: control.current ? Theme.brandText : Theme.brandMuted
            font.pixelSize: Theme.fontBody
            font.weight: control.current ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
        }
        Rectangle {
            Layout.rightMargin: Theme.gapS
            Layout.preferredWidth: Math.max(20, count.implicitWidth + 10)
            Layout.preferredHeight: 20
            radius: 10
            color: Theme.brandMarker
            visible: control.badge > 0

            Text {
                id: count

                anchors.centerIn: parent
                text: control.badge
                color: Theme.brand
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
            }
        }
    }
}
