import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Ui

// One of a few equal ways forward: an icon, what it is, and one line on what happens.
AbstractButton {
    id: control

    required property string iconName
    required property string title
    property string description

    implicitHeight: content.implicitHeight + 2 * Theme.gapL
    implicitWidth: 280
    hoverEnabled: true
    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.description: description

    background: Rectangle {
        radius: Theme.radius
        color: control.down ? Theme.selected : control.hovered ? Theme.hover : Theme.surface
        border.width: 1
        border.color: control.hovered ? Theme.accent : Theme.outline

        FocusRing {
            shown: control.visualFocus
            radius: Theme.radius + 3
        }
    }
    contentItem: Item {
        RowLayout {
            id: content

            anchors.fill: parent
            anchors.margins: Theme.gapL
            spacing: Theme.gapL

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                Layout.alignment: Qt.AlignTop
                radius: 22
                color: Theme.toneFill("info")

                Icon {
                    anchors.centerIn: parent
                    name: control.iconName
                    size: 22
                    color: Theme.toneText("info")
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: control.title
                    color: Theme.text
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Medium
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: control.description
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                }
            }
            Icon {
                Layout.alignment: Qt.AlignVCenter
                name: "chevronRight"
                size: 20
                color: Theme.textSecondary
            }
        }
    }
}
