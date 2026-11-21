pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// The frame after signing in: where you can go on the left, where you are and for which pgroup on top.
FocusScope {
    id: shell

    enum Page { Overview, Archive, Retrieve, Activity, Settings, Help }

    property int page: Shell.Overview
    readonly property var titles: [qsTr("Overview"), qsTr("Archive data"), qsTr("Retrieve data"), qsTr("Activity"), qsTr("Settings"), qsTr("Help")]

    signal notify(string message)
    // a change that can be taken back: shown as a brief message with an "Undo" that calls undo()
    signal undoOffered(string message, var undo)

    function open(target: int) {
        // PI details typed in for a pgroup without a proposal are kept from the moment the Overview is left with
        // them, as in the Widgets GUI, not only once a dataset is sent
        if (page === Shell.Overview && target !== Shell.Overview && Backend.workspace.archiveReady) {
            const warning = Backend.workspace.rememberPi()
            if (warning !== "")
                shell.notify(warning)
        }
        page = target
        Qt.callLater(shell.focusPage)
    }

    // Keyboard focus goes with the page: into the field the page names, else to the page as a whole, from where
    // Tab enters it.
    function focusPage() {
        const holder = pages.children[shell.page]
        const content = holder instanceof Loader ? holder.item : holder
        const field = content ? content.initialFocusItem : null
        if (field)
            field.forceActiveFocus()
        else if (holder)
            holder.forceActiveFocus()
    }

    Shortcut {
        sequence: "Ctrl+1"
        context: Qt.ApplicationShortcut
        enabled: !Modals.any
        onActivated: shell.open(Shell.Overview)
    }
    Shortcut {
        sequence: "Ctrl+2"
        context: Qt.ApplicationShortcut
        enabled: !Modals.any
        onActivated: shell.open(Shell.Archive)
    }
    Shortcut {
        sequence: "Ctrl+3"
        context: Qt.ApplicationShortcut
        enabled: !Modals.any
        onActivated: shell.open(Shell.Retrieve)
    }
    Shortcut {
        sequence: "Ctrl+4"
        context: Qt.ApplicationShortcut
        enabled: !Modals.any
        onActivated: shell.open(Shell.Activity)
    }
    Shortcut {
        sequences: [StandardKey.Preferences]
        context: Qt.ApplicationShortcut
        enabled: !Modals.any
        onActivated: shell.open(Shell.Settings)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 216
            color: Theme.brand

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.gapM
                spacing: Theme.gapXs

                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.gapL
                    Layout.leftMargin: Theme.gapXs
                    spacing: Theme.gapS

                    Image {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        source: "icons/scicat-logo.svg"
                        sourceSize: Qt.size(72, 72)
                        Accessible.ignored: true
                    }
                    Label {
                        text: qsTr("SciCat")
                        color: Theme.brandText
                        font.pixelSize: Theme.fontHeading
                        font.weight: Font.DemiBold
                    }
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navOverview"
                    text: qsTr("Overview")
                    iconName: "home"
                    current: shell.page === Shell.Overview
                    onClicked: shell.open(Shell.Overview)
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navArchive"
                    text: qsTr("Archive")
                    iconName: "upload"
                    current: shell.page === Shell.Archive
                    onClicked: shell.open(Shell.Archive)
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navRetrieve"
                    text: qsTr("Retrieve")
                    iconName: "download"
                    current: shell.page === Shell.Retrieve
                    onClicked: shell.open(Shell.Retrieve)
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navActivity"
                    text: qsTr("Activity")
                    iconName: "activity"
                    badge: Backend.activity.runningCount
                    current: shell.page === Shell.Activity
                    onClicked: shell.open(Shell.Activity)
                }
                Item {
                    Layout.fillHeight: true
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navSettings"
                    text: qsTr("Settings")
                    iconName: "settings"
                    current: shell.page === Shell.Settings
                    onClicked: shell.open(Shell.Settings)
                }
                NavButton {
                    Layout.fillWidth: true
                    objectName: "navHelp"
                    text: qsTr("Help")
                    iconName: "help"
                    current: shell.page === Shell.Help
                    onClicked: shell.open(Shell.Help)
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: Theme.gapS
                    Layout.bottomMargin: Theme.gapS
                    color: Theme.brandRaised
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.gapXs
                    spacing: Theme.gapS

                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: 16
                        color: Theme.brandRaised

                        Icon {
                            anchors.centerIn: parent
                            name: "user"
                            size: 18
                            color: Theme.brandText
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            Layout.fillWidth: true
                            objectName: "userName"
                            text: Backend.session.username
                            color: Theme.brandText
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: Backend.session.apiHost
                            color: Theme.brandMuted
                            font.pixelSize: Theme.fontSmall
                            elide: Text.ElideMiddle
                        }
                    }
                    IconButton {
                        objectName: "signOutButton"
                        iconName: "logout"
                        label: qsTr("Sign out")
                        iconColor: Theme.brandMuted
                        onClicked: Backend.activity.runningCount > 0 ? signOutDialog.open() : Backend.session.signOut()
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.surface

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.gapXl
                    anchors.rightMargin: Theme.gapL
                    spacing: Theme.gapM

                    Label {
                        Layout.fillWidth: true
                        objectName: "pageTitle"
                        text: shell.titles[shell.page]
                        color: Theme.text
                        font.pixelSize: Theme.fontHeading
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                        Accessible.role: Accessible.Heading
                    }
                    AppButton {
                        objectName: "groupButton"
                        visible: shell.page !== Shell.Overview
                        text: Backend.workspace.pgroup !== "" ? Backend.workspace.pgroup : qsTr("Choose a pgroup")
                        iconName: "people"
                        variant: AppButton.Tonal
                        Accessible.name: Backend.workspace.pgroup !== "" ? qsTr("Pgroup %1, change").arg(Backend.workspace.pgroup) : qsTr("Choose a pgroup")
                        onClicked: shell.open(Shell.Overview)
                    }
                    StatusTag {
                        objectName: "environmentTag"
                        text: Backend.session.environment === Session.Production ? qsTr("Production")
                            : Backend.session.environment === Session.LocalTest ? qsTr("Local test") : qsTr("%1: test data").arg(Backend.session.environmentLabel)
                        tone: Backend.session.environment === Session.Production ? "neutral"
                            : Backend.session.environment === Session.QA ? "qa" : Backend.session.environment === Session.Development ? "development" : "local"
                        strong: Backend.session.environment !== Session.Production
                    }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.outline
                }
            }

            StackLayout {
                id: pages

                function moveFocus(forward: bool) {
                    const from = pages.Window.activeFocusItem ?? pages.children[shell.page]
                    const next = from.nextItemInFocusChain(forward)
                    if (next)
                        next.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                }

                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: shell.page
                // Reached only while the page itself has the focus and none of its controls: after changing pages.
                Keys.onTabPressed: pages.moveFocus(true)
                Keys.onBacktabPressed: pages.moveFocus(false)

                OverviewPage {
                    focus: true
                    onArchiveRequested: shell.open(Shell.Archive)
                    onRetrieveRequested: shell.open(Shell.Retrieve)
                }
                Loader {
                    active: shell.page === Shell.Archive || item !== null
                    sourceComponent: ArchivePage {
                        onOverviewRequested: shell.open(Shell.Overview)
                        onRetrieveRequested: shell.open(Shell.Retrieve)
                        onNotify: message => shell.notify(message)
                        onUndoOffered: (message, undo) => shell.undoOffered(message, undo)
                    }
                }
                Loader {
                    active: shell.page === Shell.Retrieve || item !== null
                    sourceComponent: RetrievePage {
                        onOverviewRequested: shell.open(Shell.Overview)
                        onArchiveRequested: shell.open(Shell.Archive)
                        onActivityRequested: shell.open(Shell.Activity)
                        onNotify: message => shell.notify(message)
                    }
                }
                Loader {
                    active: shell.page === Shell.Activity || item !== null
                    sourceComponent: ActivityPage {
                        onNotify: message => shell.notify(message)
                    }
                }
                Loader {
                    active: shell.page === Shell.Settings || item !== null
                    sourceComponent: SettingsForm {
                        objectName: "settingsPage"
                        showServer: true
                    }
                }
                Loader {
                    active: shell.page === Shell.Help || item !== null
                    sourceComponent: HelpPage {}
                }
            }
        }
    }

    AppDialog {
        id: signOutDialog

        objectName: "signOutDialog"
        width: Math.min(460, parent.width - 2 * Theme.gapXl)
        title: Backend.activity.runningCount === 1 ? qsTr("Sign out while 1 task is still running?") : qsTr("Sign out while %1 tasks are still running?").arg(Backend.activity.runningCount)
        initialFocusItem: signOutStay
        // tasks that have ended meanwhile leave nothing to ask
        relevant: Backend.activity.runningCount > 0

        Label {
            width: parent.width
            text: qsTr("Signing out stops the ingestions and copies that are running now.")
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
                objectName: "signOutAnyway"
                text: qsTr("Stop them and sign out")
                variant: AppButton.Danger
                onClicked: {
                    signOutDialog.close()
                    Backend.session.signOut()
                }
            }
            AppButton {
                id: signOutStay

                Layout.margins: Theme.gapM
                objectName: "signOutStay"
                text: qsTr("Stay signed in")
                variant: AppButton.Filled
                onClicked: signOutDialog.close()
            }
        }
    }
}
