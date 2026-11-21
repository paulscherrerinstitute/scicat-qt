import QtQuick

// Keyboard focus that is visible on every surface: place it in a control and bind "shown" to its visualFocus.
Rectangle {
    property bool shown: false
    property real inset: -3

    anchors.fill: parent
    anchors.margins: inset
    radius: Theme.radiusSmall + 3
    color: "transparent"
    border.width: 2
    border.color: Theme.focus
    visible: shown
}
