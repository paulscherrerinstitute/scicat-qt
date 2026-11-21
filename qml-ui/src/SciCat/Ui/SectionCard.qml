import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A titled surface that groups what belongs together.
Rectangle {
    id: root

    property string title
    property string subtitle
    property int padding: Theme.gapL
    default property alias content: body.data
    property alias headerItems: headerSlot.data

    implicitHeight: layout.implicitHeight + 2 * padding
    implicitWidth: 320
    radius: Theme.radius
    color: Theme.surface
    border.width: 1
    border.color: Theme.outline

    ColumnLayout {
        id: layout

        anchors.fill: parent
        anchors.margins: root.padding
        spacing: Theme.gapM

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gapS
            visible: root.title !== ""

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.title
                    color: Theme.text
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Medium
                    wrapMode: Text.WordWrap
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
                id: headerSlot

                spacing: Theme.gapS
            }
        }
        ColumnLayout {
            id: body

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.gapM
        }
    }
}
