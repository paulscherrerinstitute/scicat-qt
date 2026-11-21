import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Ui

// One dataset in the list: a box to select it, its name and ID, its state as a word, size, date and kind.
Item {
    id: row

    required property int index
    required property string pid
    required property string name
    required property string type
    required property string sizeText
    required property string createdText
    required property int archiveState
    required property bool selected
    required property string jobNote

    property string stateText
    property string stateTone
    property string stateIcon
    property bool current: false
    property bool focused: false
    property int stateWidth: 138
    property int sizeWidth: 86
    property int createdWidth: 124
    property int kindWidth: 64
    property bool showSize: true
    property bool showCreated: true
    property bool showKind: true

    signal activated()
    signal toggled()

    width: ListView.view ? ListView.view.width : 0
    height: 52
    objectName: "datasetRow"
    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("%1, %2, %3, created %4").arg(name).arg(stateText).arg(sizeText).arg(createdText)
    Accessible.selected: selected

    Rectangle {
        anchors.fill: parent
        color: row.current ? Theme.selected : hover.hovered ? Theme.hover : "transparent"

        Rectangle {
            width: 3
            height: parent.height
            color: Theme.accent
            visible: row.current
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.width: 2
            border.color: Theme.focus
            visible: row.focused
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.outline
        }
    }
    HoverHandler {
        id: hover
    }
    TapHandler {
        onTapped: row.activated()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.gapS
        anchors.rightMargin: Theme.gapL
        spacing: Theme.gapS

        CheckBox {
            Layout.preferredWidth: 36
            objectName: "datasetRowCheck"
            checked: row.selected
            focusPolicy: Qt.NoFocus
            Accessible.name: qsTr("Select %1").arg(row.name)
            onClicked: row.toggled()
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                id: nameLabel

                Layout.fillWidth: true
                Layout.leftMargin: 6
                text: row.name
                color: Theme.text
                font.pixelSize: Theme.fontBody
                font.weight: Font.Medium
                elide: Text.ElideRight
                // a name that does not fit is shown in full while pointed at
                ToolTip.text: row.name
                ToolTip.visible: truncated && hover.hovered
                ToolTip.delay: 600
            }
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 6
                text: row.pid
                color: Theme.textSecondary
                font.family: Theme.monoFamily
                font.pixelSize: 11
                elide: Text.ElideMiddle
            }
        }
        Item {
            Layout.preferredWidth: row.stateWidth
            Layout.fillHeight: true

            StatusTag {
                anchors.verticalCenter: parent.verticalCenter
                x: 6
                objectName: "datasetRowState"
                text: row.stateText
                tone: row.stateTone
                iconName: row.stateIcon
            }
        }
        Label {
            Layout.preferredWidth: row.sizeWidth
            visible: row.showSize
            text: row.sizeText
            color: Theme.text
            horizontalAlignment: Text.AlignRight
            rightPadding: 6
        }
        Label {
            Layout.preferredWidth: row.createdWidth
            visible: row.showCreated
            leftPadding: 6
            text: row.createdText
            color: Theme.textSecondary
            elide: Text.ElideRight
        }
        Label {
            Layout.preferredWidth: row.kindWidth
            visible: row.showKind
            text: row.type === "raw" ? qsTr("Raw") : row.type === "derived" ? qsTr("Derived") : row.type
            color: Theme.textSecondary
            elide: Text.ElideRight
        }
    }
}
