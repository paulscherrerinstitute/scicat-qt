import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SciCat.Ui

// One numbered step of a how-to.
RowLayout {
    id: root

    property int number: 1
    property string text

    spacing: Theme.gapM
    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Step %1: %2").arg(number).arg(text)

    Rectangle {
        Layout.preferredWidth: 24
        Layout.preferredHeight: 24
        Layout.alignment: Qt.AlignTop
        radius: 12
        color: Theme.toneFill("info")

        Text {
            anchors.centerIn: parent
            text: root.number
            color: Theme.toneText("info")
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
        }
    }
    Label {
        Layout.fillWidth: true
        text: root.text
        color: Theme.text
        wrapMode: Text.WordWrap
        Accessible.ignored: true
    }
}
