import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Brief confirmations at the bottom of the window. They stay ten seconds, wait while pointed at or focused,
// can be closed, and may offer one action such as Undo, which the platform's Undo key triggers as well.
// Problems never go here: they stay where they belong.
Item {
    id: root

    property string text
    property string actionText
    property var action: null
    // asked for while a dialog was open; shown once the dialog has closed, as it could not be used under it
    property var waiting: null
    // the Undo key belongs to a text field while one is being typed in
    readonly property bool typing: Window.activeFocusItem instanceof TextInput || Window.activeFocusItem instanceof TextEdit

    function show(message: string, actionLabel: string, callback: var) {
        if (Modals.any) {
            waiting = { message: message, actionLabel: actionLabel, callback: callback }
            return
        }
        waiting = null
        text = message
        actionText = actionLabel ?? ""
        action = callback ?? null
        timer.restart()
        // also for those who cannot see it appear
        label.Accessible.announce(action ? qsTr("%1 Press %2 to undo.").arg(message).arg(undoKey.nativeText) : message)
    }

    function hide() {
        timer.stop()
        waiting = null
        text = ""
        action = null
    }

    function trigger() {
        const callback = root.action
        root.hide()
        if (callback)
            callback()
    }

    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.bottomMargin: Theme.gapXl
    width: Math.min(row.implicitWidth + Theme.gapL + Theme.gapS, (parent ? parent.width : 600) - 2 * Theme.gapXl)
    height: row.implicitHeight + 2 * Theme.gapS
    visible: text !== ""
    z: 100

    Connections {
        target: Modals

        function onAnyChanged() {
            const held = root.waiting
            if (!Modals.any && held)
                root.show(held.message, held.actionLabel, held.callback)
        }
    }
    Shortcut {
        id: undoKey

        sequences: [StandardKey.Undo]
        context: Qt.ApplicationShortcut
        enabled: root.visible && root.action !== null && !root.typing && !Modals.any
        onActivated: root.trigger()
    }
    Timer {
        id: timer

        interval: 10000
        running: false
        onTriggered: {
            if (hover.hovered || button.activeFocus || closeButton.activeFocus)
                restart()
            else
                root.hide()
        }
    }
    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.dark ? "#E8ECEE" : "#1B2124"

        HoverHandler {
            id: hover
        }
    }
    RowLayout {
        id: row

        anchors.fill: parent
        anchors.leftMargin: Theme.gapL
        anchors.rightMargin: Theme.gapS
        anchors.topMargin: Theme.gapS
        anchors.bottomMargin: Theme.gapS
        spacing: Theme.gapM

        Label {
            id: label

            Layout.fillWidth: true
            objectName: "toastText"
            text: root.text
            color: Theme.dark ? "#1B2124" : "#FFFFFF"
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Notification
            Accessible.name: root.text
        }
        AppButton {
            id: button

            objectName: "toastAction"
            text: root.actionText
            visible: text !== ""
            variant: AppButton.Tonal
            onClicked: root.trigger()
        }
        IconButton {
            id: closeButton

            objectName: "toastClose"
            iconName: "close"
            label: qsTr("Close this message")
            iconColor: Theme.dark ? "#1B2124" : "#FFFFFF"
            onClicked: root.hide()
        }
    }
}
