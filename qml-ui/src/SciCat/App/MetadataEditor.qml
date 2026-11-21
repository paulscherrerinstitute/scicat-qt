pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import SciCat.Backend
import SciCat.Ui

// Scientific metadata as an indented list. With "editable" off it is the read-only view used for archived datasets,
// whose rows are two texts and nothing else: a dataset can have hundreds of entries.
ColumnLayout {
    id: editor

    required property MetadataModel metadata
    property bool editable: true
    // problems are shown for entries the user has filled in, and for all of them once this is set
    property bool showAllProblems: false

    // the row that takes keyboard focus as soon as the list has made it
    property int focusRow: -1

    function focusEntry(row: int) {
        if (row < 0)
            return
        // the list takes changes of the model in once per frame; until then its rows still have their old numbers
        list.forceLayout()
        const item = list.itemAtIndex(row)
        if (item)
            item.takeFocus() // qmllint disable missing-property
        else
            focusRow = row
    }

    function focusFirstProblem() {
        focusEntry(metadata.revealFirstProblem())
    }

    spacing: Theme.gapS

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.gapS
        visible: editor.editable

        AppButton {
            objectName: "metadataAddField"
            text: qsTr("Add entry")
            iconName: "plus"
            onClicked: editor.focusEntry(editor.metadata.addField(-1))
        }
        AppButton {
            objectName: "metadataAddGroup"
            text: qsTr("Add group")
            iconName: "folder"
            onClicked: editor.focusEntry(editor.metadata.addGroup(-1))
        }
        Item {
            Layout.fillWidth: true
        }
    }

    Label {
        Layout.fillWidth: true
        objectName: "metadataEmpty"
        visible: editor.metadata.empty
        text: editor.editable ? qsTr("No entries. Add what describes the measurement: sample, beam energy, detector and so on.") : qsTr("No scientific metadata.")
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
    }

    ListView {
        id: list

        Layout.fillWidth: true
        Layout.preferredHeight: contentHeight
        objectName: "metadataList"
        interactive: false
        model: editor.metadata
        visible: !editor.metadata.empty
        delegate: editor.editable ? editRow : readRow
        Accessible.role: Accessible.Tree
        Accessible.name: qsTr("Scientific metadata")
    }

    Component {
        id: readRow

        Item {
            id: row

            required property int index
            required property string key
            required property string value
            required property int kind
            required property int depth
            required property bool container
            required property bool expanded
            required property int childCount

            width: ListView.view.width
            // a long value takes the lines it needs
            height: Math.max(30, valueLabel.implicitHeight + 12)
            objectName: "metadataRow"
            Accessible.role: Accessible.TreeItem
            Accessible.name: !container ? qsTr("%1: %2").arg(key).arg(valueLabel.text)
                : childCount === 1 ? qsTr("%1, group with 1 entry").arg(key) : qsTr("%1, group with %2 entries").arg(key).arg(childCount)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: row.depth * 22
                spacing: Theme.gapS

                Loader {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 2
                    active: row.container
                    sourceComponent: IconButton {
                        iconName: row.expanded ? "chevronDown" : "chevronRight"
                        label: row.expanded ? qsTr("Collapse %1").arg(row.key) : qsTr("Expand %1").arg(row.key)
                        onClicked: editor.metadata.toggle(row.index)
                    }
                }
                Label {
                    Layout.preferredWidth: Math.max(90, Math.min(220, row.width * 0.45) - row.depth * 22)
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 6
                    text: row.key
                    color: row.container ? Theme.text : Theme.textSecondary
                    font.weight: row.container ? Font.Medium : Font.Normal
                    elide: Text.ElideRight
                    ToolTip.text: row.key
                    ToolTip.visible: truncated && keyHover.hovered
                    ToolTip.delay: 600

                    HoverHandler {
                        id: keyHover
                    }
                }
                Label {
                    id: valueLabel

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 6
                    text: row.container ? "" : row.kind === MetadataModel.YesNo ? (row.value === "true" ? qsTr("yes") : qsTr("no")) : row.value
                    color: Theme.text
                    wrapMode: Text.Wrap
                    maximumLineCount: 6
                    elide: Text.ElideRight
                }
            }
        }
    }

    Component {
        id: editRow

        Item {
            id: row

            required property int index
            required property string key
            required property string value
            required property int kind
            required property int depth
            required property bool container
            required property bool expanded
            required property int childCount
            required property bool keyEditable
            required property string keyProblem
            required property string valueProblem

            readonly property string shownProblem: (editor.showAllProblems || key !== "" ? keyProblem : "") !== "" ? keyProblem
                : (editor.showAllProblems || value !== "" ? valueProblem : "")

            // the name while it is missing or wrong, else the value
            function takeFocus() {
                const field = keyField.enabled && (row.keyProblem !== "" || !valueField.visible) ? keyField : valueField.visible ? valueField : keyField
                field.forceActiveFocus(Qt.TabFocusReason)
            }

            width: ListView.view.width
            height: shownProblem !== "" ? 70 : 52
            objectName: "metadataRow"
            Component.onCompleted: {
                if (editor.focusRow === index) {
                    editor.focusRow = -1
                    takeFocus()
                }
            }
            Accessible.role: Accessible.TreeItem
            Accessible.name: !container ? qsTr("%1: %2").arg(key).arg(value)
                : childCount === 1 ? qsTr("%1, group with 1 entry").arg(key) : qsTr("%1, group with %2 entries").arg(key).arg(childCount)

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: row.depth * 22
                height: 52
                spacing: Theme.gapS

                IconButton {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    iconName: row.expanded ? "chevronDown" : "chevronRight"
                    label: row.expanded ? qsTr("Collapse %1").arg(row.key) : qsTr("Expand %1").arg(row.key)
                    opacity: row.container ? 1 : 0
                    enabled: row.container
                    onClicked: editor.metadata.toggle(row.index)
                }
                AppTextField {
                    id: keyField

                    Layout.preferredWidth: Math.max(120, 220 - row.depth * 22)
                    Layout.preferredHeight: 44
                    objectName: "metadataKey"
                    enabled: row.keyEditable
                    text: row.key
                    Accessible.name: qsTr("Name of the entry")
                    onTextEdited: editor.metadata.setKey(row.index, text)
                }
                AppComboBox {
                    Layout.preferredWidth: 118
                    Layout.preferredHeight: 44
                    objectName: "metadataKind"
                    model: [qsTr("Text"), qsTr("Number"), qsTr("Yes / no"), qsTr("Group"), qsTr("List")]
                    currentIndex: row.kind
                    Accessible.name: qsTr("Kind of value of %1").arg(row.key)
                    onActivated: index => editor.metadata.setKind(row.index, index)
                }
                AppTextField {
                    id: valueField

                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    objectName: "metadataValue"
                    visible: !row.container && row.kind !== MetadataModel.YesNo
                    text: row.value
                    horizontalAlignment: row.kind === MetadataModel.Number ? TextInput.AlignRight : TextInput.AlignLeft
                    Accessible.name: qsTr("Value of %1").arg(row.key)
                    onTextEdited: editor.metadata.setValue(row.index, text)
                }
                Switch {
                    objectName: "metadataValueSwitch"
                    visible: row.kind === MetadataModel.YesNo
                    checked: row.value === "true"
                    Accessible.name: qsTr("Value of %1").arg(row.key)
                    onToggled: editor.metadata.setValue(row.index, checked ? "true" : "false")
                }
                Label {
                    Layout.fillWidth: true
                    visible: row.container || row.kind === MetadataModel.YesNo
                    text: !row.container ? (row.value === "true" ? qsTr("yes") : qsTr("no"))
                        : row.childCount === 1 ? qsTr("1 entry") : qsTr("%1 entries").arg(row.childCount)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }
                IconButton {
                    objectName: "metadataAddChild"
                    visible: row.container
                    iconName: "plus"
                    label: qsTr("Add an entry to %1").arg(row.key)
                    onClicked: editor.focusEntry(editor.metadata.addField(row.index))
                }
                IconButton {
                    objectName: "metadataRemove"
                    iconName: "trash"
                    label: !row.container || row.childCount === 0 ? qsTr("Remove %1").arg(row.key)
                        : row.childCount === 1 ? qsTr("Remove %1 and its entry").arg(row.key) : qsTr("Remove %1 and its %2 entries").arg(row.key).arg(row.childCount)
                    onClicked: editor.metadata.removeAt(row.index)
                }
            }
            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: row.depth * 22 + 34
                anchors.bottomMargin: 2
                objectName: "metadataRowProblem"
                visible: row.shownProblem !== ""
                text: row.shownProblem
                color: Theme.toneText("error")
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
                Accessible.role: Accessible.AlertMessage
            }
        }
    }
}
