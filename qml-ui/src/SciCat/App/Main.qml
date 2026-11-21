pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

ApplicationWindow {
    id: window

    readonly property string environmentSuffix: Backend.session.environment === Session.Production ? "" : " (" + Backend.session.environmentLabel + ")"
    property bool closingConfirmed: false
    // "Stop them and quit" was chosen: the window closes once the tasks have ended, or after a few seconds
    property bool quitting: false

    function notify(message: string) {
        toasts.show(message, "", null)
    }

    // Qt Quick leaves keyboard focus on an item that gets hidden, and gives it to one that already is, where
    // typing would change something unseen. Take the focus off, up to the first item that can still be seen.
    function dropHiddenFocus() {
        for (let item = window.activeFocusItem; item && !item.visible; item = item.parent)
            item.focus = false
    }

    function finishQuit() {
        quitTimeout.stop()
        closingConfirmed = true
        close()
    }

    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 620
    visible: !Backend.startHidden
    title: qsTr("SciCat") + environmentSuffix + (Backend.windowTag !== "" ? " [" + Backend.windowTag + "]" : "")
    color: Theme.window
    font.pixelSize: Theme.fontBody

    Material.theme: Material.System
    Material.accent: Theme.accent
    Material.primary: Theme.brand
    Material.background: Theme.window
    Material.foreground: Theme.text

    onClosing: close => {
        if (!closingConfirmed && (quitting || Backend.activity.runningCount > 0)) {
            close.accepted = false
            if (!quitting)
                quitDialog.open()
            return
        }
        if (!Backend.startHidden && visibility === Window.Windowed) {
            Backend.settings.setUi("windowWidth", width)
            Backend.settings.setUi("windowHeight", height)
        }
    }
    // after the change has gone through, not in the middle of it
    onActiveFocusItemChanged: Qt.callLater(window.dropHiddenFocus)
    Component.onCompleted: {
        const storedWidth = Backend.settings.ui("windowWidth", 0)
        const storedHeight = Backend.settings.ui("windowHeight", 0)
        if (!Backend.startHidden && storedWidth >= minimumWidth && storedHeight >= minimumHeight) {
            width = Math.min(storedWidth, Screen.desktopAvailableWidth)
            height = Math.min(storedHeight, Screen.desktopAvailableHeight)
        }
    }

    Connections {
        target: window.activeFocusItem

        function onVisibleChanged() {
            window.dropHiddenFocus()
        }
    }
    Connections {
        target: Backend.session

        // a message of the user who leaves, and its "Undo", are not for whoever signs in next
        function onAboutToSignOut() {
            toasts.hide()
        }
    }
    Connections {
        target: Backend.activity

        function onRunningCountChanged() {
            if (window.quitting && Backend.activity.runningCount === 0)
                window.finishQuit()
        }
    }
    Timer {
        id: quitTimeout

        // a program that does not stop when asked is ended with the application
        interval: 4000
        onTriggered: window.finishQuit()
    }

    Loader {
        id: pageLoader

        anchors.fill: parent
        sourceComponent: Backend.session.signedIn ? shellPage : loginPage
        onLoaded: {
            (item as Item).forceActiveFocus()
            const shell = item as Shell
            if (shell)
                shell.focusPage()
        }
    }
    Component {
        id: loginPage

        LoginPage {
            onSettingsRequested: settingsDialog.open()
        }
    }
    Component {
        id: shellPage

        Shell {
            onNotify: message => window.notify(message)
            onUndoOffered: (message, undo) => toasts.show(message, qsTr("Undo"), undo)
        }
    }

    ToastHost {
        id: toasts
    }

    AppDialog {
        id: settingsDialog

        objectName: "settingsDialog"
        width: Math.min(window.width - 2 * Theme.gapXl, 720)
        height: Math.min(window.height - 2 * Theme.gapXl, 640)
        title: qsTr("Settings")
        padding: 0
        initialFocusItem: settingsClose

        SettingsForm {
            anchors.fill: parent
            objectName: "settingsForm"
            showServer: false
        }
        footer: RowLayout {
            Item {
                Layout.fillWidth: true
            }
            AppButton {
                id: settingsClose

                Layout.margins: Theme.gapM
                objectName: "settingsDialogClose"
                text: qsTr("Close")
                variant: AppButton.Filled
                onClicked: settingsDialog.close()
            }
        }
    }

    AppDialog {
        id: quitDialog

        objectName: "quitDialog"
        width: Math.min(460, window.width - 2 * Theme.gapXl)
        title: Backend.activity.runningCount === 1 ? qsTr("Quit while 1 task is still running?") : qsTr("Quit while %1 tasks are still running?").arg(Backend.activity.runningCount)
        initialFocusItem: quitCancel
        // tasks that have ended meanwhile leave nothing to ask
        relevant: Backend.activity.runningCount > 0

        Label {
            width: parent.width
            text: qsTr("Quitting stops them. A dataset that scicat-cli has already registered stays registered; files copied so far stay where they are.")
            wrapMode: Text.WordWrap
            color: Theme.text
        }
        footer: RowLayout {
            spacing: Theme.gapS

            Item {
                Layout.fillWidth: true
            }
            AppButton {
                Layout.topMargin: Theme.gapM
                Layout.bottomMargin: Theme.gapM
                objectName: "quitAnyway"
                text: window.quitting ? qsTr("Stopping…") : qsTr("Stop them and quit")
                variant: AppButton.Danger
                busy: window.quitting
                enabled: !window.quitting
                onClicked: {
                    // the programs get the time to end; the window closes when they have
                    window.quitting = true
                    quitTimeout.start()
                    Backend.stopAll()
                    if (Backend.activity.runningCount === 0)
                        window.finishQuit()
                }
            }
            AppButton {
                id: quitCancel

                Layout.margins: Theme.gapM
                objectName: "quitCancel"
                text: qsTr("Keep running")
                variant: AppButton.Filled
                enabled: !window.quitting
                onClicked: quitDialog.close()
            }
        }
    }
}
