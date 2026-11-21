import QtQuick

// A search field with its own clear button. What to search for is said a moment after the typing has paused, so a
// long list is filtered once and not once per letter; at once when the box is emptied or Return is pressed.
AppTextField {
    id: control

    property string label: qsTr("Search")
    // while a text set by show() is put in: that is not typing
    property bool showing: false

    signal searchRequested(string text)

    function apply() {
        delay.stop()
        searchRequested(text)
    }

    // The search that is in effect, when it was set or dropped somewhere else. Typed text that still waits to be
    // applied is left alone.
    function show(applied: string) {
        if (delay.running || text.trim() === applied)
            return
        showing = true
        text = applied
        showing = false
    }

    implicitWidth: 260
    leftPadding: 38
    rightPadding: clear.visible ? 36 : 12
    placeholderText: activeFocus || text !== "" ? "" : label
    Accessible.name: label
    Accessible.searchEdit: true
    onTextChanged: {
        if (showing)
            return
        if (text === "")
            apply()
        else
            delay.restart()
    }
    onAccepted: apply()

    Timer {
        id: delay

        interval: 150
        onTriggered: control.searchRequested(control.text)
    }
    Icon {
        x: 12
        anchors.verticalCenter: parent.verticalCenter
        name: "search"
        size: 18
        color: Theme.textSecondary
    }
    IconButton {
        id: clear

        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        iconName: "close"
        label: qsTr("Clear the search")
        visible: control.text !== ""
        onClicked: {
            control.clear()
            control.forceActiveFocus()
        }
    }
}
