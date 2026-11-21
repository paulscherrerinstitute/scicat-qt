import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Signing in: which SciCat, and the token from its website.
FocusScope {
    id: page

    readonly property Session session: Backend.session
    readonly property bool busy: session.state === Session.SigningIn
    readonly property bool localTest: session.environment === Session.LocalTest

    signal settingsRequested()

    function submit() {
        session.signIn(tokenField.text)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: Math.max(320, page.width * 0.38)
            color: Theme.brand

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.gapXxl
                spacing: Theme.gapM

                Item {
                    Layout.fillHeight: true
                }
                Image {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 88
                    source: "icons/scicat-logo.svg"
                    sourceSize: Qt.size(176, 176)
                    Accessible.ignored: true
                }
                Label {
                    text: qsTr("SciCat")
                    color: Theme.brandText
                    font.pixelSize: 34
                    font.weight: Font.DemiBold
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Archive research data at PSI and get it back.")
                    color: Theme.brandMuted
                    font.pixelSize: Theme.fontTitle
                    wrapMode: Text.WordWrap
                }
                Item {
                    Layout.fillHeight: true
                    Layout.preferredHeight: 2
                }
                Label {
                    text: qsTr("Version %1").arg(Backend.version)
                    color: Theme.brandMuted
                    font.pixelSize: Theme.fontSmall
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 2 * Theme.gapXxl, 420)
                spacing: Theme.gapL

                Label {
                    text: qsTr("Sign in")
                    color: Theme.text
                    font.pixelSize: Theme.fontDisplay
                    font.weight: Font.Medium
                    Accessible.role: Accessible.Heading
                }

                InlineMessage {
                    Layout.fillWidth: true
                    objectName: "localTestNote"
                    visible: page.localTest
                    tone: "info"
                    title: qsTr("Local test backend")
                    text: qsTr("Every server points to %1. No real SciCat is contacted. The token is: good").arg(page.session.apiHost)
                }

                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Server")
                    help: page.localTest ? "" : page.session.apiHost

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapM

                        AppComboBox {
                            id: serverBox

                            Layout.fillWidth: true
                            objectName: "serverBox"
                            model: page.session.serverNames
                            currentIndex: page.session.serverId
                            enabled: !page.busy
                            Accessible.name: qsTr("Server")
                            onActivated: index => page.session.serverId = index
                        }
                        StatusTag {
                            objectName: "loginEnvironmentTag"
                            visible: page.session.environment !== Session.Production
                            text: page.localTest ? qsTr("Local test") : qsTr("Test data only")
                            tone: page.localTest ? "local" : page.session.environment === Session.QA ? "qa" : "development"
                            strong: true
                        }
                    }
                }

                FormField {
                    Layout.fillWidth: true
                    label: qsTr("Token")
                    problem: page.session.problem === Session.TokenMissing ? page.session.problemText : ""
                    help: page.localTest ? "" : qsTr("Paste the token from your SciCat user page.")

                    AppTextField {
                        id: tokenField

                        Layout.fillWidth: true
                        objectName: "tokenField"
                        echoMode: reveal.checked ? TextInput.Normal : TextInput.Password
                        rightPadding: 44
                        enabled: !page.busy
                        focus: true
                        Accessible.name: qsTr("Token")
                        Accessible.passwordEdit: !reveal.checked
                        onTextEdited: page.session.clearProblem()
                        onAccepted: page.submit()

                        IconButton {
                            id: reveal

                            objectName: "revealToken"
                            anchors.right: parent.right
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            checkable: true
                            iconName: checked ? "eyeOff" : "eye"
                            label: checked ? qsTr("Hide the token") : qsTr("Show the token")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gapXs
                        visible: !page.localTest

                        Label {
                            text: qsTr("No token yet?")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSmall
                        }
                        AppButton {
                            objectName: "tokenLink"
                            text: qsTr("Get one at %1").arg(page.session.tokenUrlText)
                            iconName: "external"
                            variant: AppButton.Plain
                            font.pixelSize: Theme.fontSmall
                            onClicked: Backend.openUrl(page.session.tokenUrl)
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }

                InlineMessage {
                    Layout.fillWidth: true
                    objectName: "loginProblem"
                    visible: page.session.problem === Session.TokenRejected || page.session.problem === Session.NoConnection
                    tone: "error"
                    title: page.session.problem === Session.TokenRejected ? qsTr("That token did not work") : qsTr("No connection to SciCat")
                    text: page.session.problemText
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.gapM

                    AppButton {
                        objectName: "settingsButton"
                        text: qsTr("Settings")
                        iconName: "settings"
                        variant: AppButton.Plain
                        onClicked: page.settingsRequested()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    AppButton {
                        objectName: "signInButton"
                        text: page.busy ? qsTr("Signing in…") : qsTr("Sign in")
                        variant: AppButton.Filled
                        busy: page.busy
                        enabled: !page.busy
                        onClicked: page.submit()
                    }
                }
            }
        }
    }

    Connections {
        target: page.session

        function onProblemChanged() {
            if (page.session.problem !== Session.NoProblem) {
                tokenField.forceActiveFocus()
                tokenField.selectAll()
            }
        }
    }
}
