import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// How the two tasks work, in the steps you actually take.
ScrollPage {
    id: page

    Item {
        implicitWidth: page.availableWidth
        implicitHeight: column.implicitHeight + 2 * Theme.gapXl

        ColumnLayout {
            id: column

            anchors.top: parent.top
            anchors.topMargin: Theme.gapXl
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width - 2 * Theme.gapXl, 760)
            spacing: Theme.gapL

            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Archive data")

                Repeater {
                    model: [
                        qsTr("On Overview, choose the pgroup. Its proposal fills in the owner and the principal investigator."),
                        qsTr("On Archive, add the folders and files of one dataset. Up to 150,000 files fit in a dataset."),
                        qsTr("Give the dataset a name, say whether it is raw or derived data, and add scientific metadata."),
                        qsTr("Check the summary and send it. scicat-cli registers the dataset in SciCat and asks the archive to store the files on tape."),
                        qsTr("If something fails, the log says why. Go back, fix it and send again: nothing you entered is lost.")
                    ]

                    HelpStep {
                        required property int index
                        required property string modelData

                        Layout.fillWidth: true
                        number: index + 1
                        text: modelData
                    }
                }
            }
            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Get data back")

                Repeater {
                    model: [
                        qsTr("On Retrieve, find the datasets. Search by name, or show only one state."),
                        qsTr("Tick the datasets that are \"In archive\" and choose \"Request from archive\". The archive copies them from tape to the retrieve server, which can take hours. SciCat sends an email when it is done."),
                        qsTr("When a dataset says \"Ready to copy\", tick it and choose \"Copy to disk…\". Copying needs a Kerberos ticket: run kinit in a terminal first."),
                        qsTr("Follow requests and copies on Activity.")
                    ]

                    HelpStep {
                        required property int index
                        required property string modelData

                        Layout.fillWidth: true
                        number: index + 1
                        text: modelData
                    }
                }
            }
            SectionCard {
                Layout.fillWidth: true
                title: qsTr("Good to know")

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Tokens expire. When SciCat stops accepting yours, sign out and get a new one at %1.").arg(Backend.session.tokenUrlText)
                    color: Theme.text
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Keys: Ctrl+1 to Ctrl+4 switch between Overview, Archive, Retrieve and Activity. On Retrieve, Ctrl+F searches and the Down key goes from the search into the list. There the arrow keys, Page Up, Page Down, Home and End move, Space ticks a dataset and Ctrl+A ticks all that are listed. Ctrl+Z takes back what a message offers to undo. On a Mac, use Command instead of Ctrl.")
                    color: Theme.text
                    wrapMode: Text.WordWrap
                }
                AppButton {
                    text: qsTr("Open SciCat in the browser")
                    iconName: "external"
                    onClicked: Backend.openUrl(Backend.session.tokenUrl)
                }
            }
        }
    }
}
