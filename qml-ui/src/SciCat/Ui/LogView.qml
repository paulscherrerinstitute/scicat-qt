import QtQuick
import QtQuick.Controls.Material

// Program output as it arrives. Follows the end unless the reader has scrolled up or is selecting something.
Rectangle {
    id: root

    property string text
    property string label: qsTr("Log")
    // what the text area holds: new output is added to it instead of setting everything again for each line
    property string shown

    implicitHeight: 220
    implicitWidth: 320
    radius: Theme.radiusSmall
    color: Theme.surfaceAlt
    border.width: 1
    border.color: area.activeFocus ? Theme.focus : Theme.outline

    function sync() {
        if (text === shown)
            return
        // asked before the text grows: afterwards the end is somewhere else
        const follow = area.atEnd && area.selectedText === ""
        if (shown !== "" && text.length > shown.length && text.startsWith(shown))
            area.insert(area.length, text.slice(shown.length))
        else
            area.text = text
        shown = text
        if (follow)
            area.cursorPosition = area.length
    }

    onTextChanged: sync()
    Component.onCompleted: sync()

    ScrollView {
        id: scroll

        anchors.fill: parent
        anchors.margins: 1

        TextArea {
            id: area

            readonly property bool atEnd: scroll.ScrollBar.vertical.position + scroll.ScrollBar.vertical.size > 0.98

            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.WrapAnywhere
            textFormat: TextEdit.PlainText
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fontSmall
            color: Theme.text
            background: null
            padding: Theme.gapM
            Accessible.name: root.label
        }
    }
}
