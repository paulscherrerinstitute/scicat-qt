import QtQuick
import QtQuick.Layouts

// A state as a word on a tinted pill. The word carries the meaning, the colour only supports it.
Rectangle {
    id: root

    property string text
    property string tone: "neutral"
    property string iconName
    property bool strong: false

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 22
    radius: 11
    color: Theme.toneFill(tone)
    // the grey of a neutral tag is close to the grey of a row that is pointed at: the edge keeps its shape there
    border.width: tone === "neutral" ? 1 : 0
    border.color: Theme.outline
    Accessible.role: Accessible.StaticText
    Accessible.name: text

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 4

        Icon {
            name: root.iconName
            size: 13
            color: Theme.toneText(root.tone)
            visible: root.iconName !== ""
        }
        Text {
            text: root.text
            color: Theme.toneText(root.tone)
            font.pixelSize: Theme.fontSmall
            font.weight: root.strong ? Font.DemiBold : Font.Medium
        }
    }
}
