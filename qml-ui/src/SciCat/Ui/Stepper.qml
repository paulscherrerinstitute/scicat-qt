pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// The steps of a task in their order: where you are, what is done, what still has a problem.
RowLayout {
    id: root

    // The titles make the buttons. What is done and what has a problem are read per step, so a change of them
    // leaves the buttons, and the keyboard focus on one of them, in place.
    property list<string> titles
    property list<bool> done
    property list<bool> problems
    property int current: 0

    signal stepRequested(int index)

    spacing: Theme.gapS
    Accessible.role: Accessible.PageTabList

    Repeater {
        model: root.titles

        RowLayout {
            id: step

            required property int index
            required property string modelData
            readonly property bool isCurrent: index === root.current
            readonly property bool isDone: root.done[index] === true
            readonly property bool hasProblem: root.problems[index] === true

            spacing: Theme.gapS

            Rectangle {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 1
                color: Theme.outlineStrong
                visible: step.index > 0
            }
            AbstractButton {
                id: button

                implicitHeight: 36
                implicitWidth: content.implicitWidth + 16
                hoverEnabled: true
                Accessible.role: Accessible.PageTab
                Accessible.selected: step.isCurrent
                Accessible.name: step.hasProblem ? qsTr("Step %1, %2, needs attention").arg(step.index + 1).arg(step.modelData)
                    : step.isDone ? qsTr("Step %1, %2, done").arg(step.index + 1).arg(step.modelData)
                                  : qsTr("Step %1, %2").arg(step.index + 1).arg(step.modelData)
                onClicked: root.stepRequested(step.index)

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: button.hovered && !step.isCurrent ? Theme.hover : "transparent"

                    FocusRing {
                        shown: button.visualFocus
                        inset: 0
                    }
                }
                contentItem: RowLayout {
                    id: content

                    spacing: Theme.gapS

                    Rectangle {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        radius: 12
                        color: step.hasProblem ? Theme.toneFill("error")
                            : step.isCurrent ? Theme.accent : step.isDone ? Theme.toneFill("success") : "transparent"
                        border.width: step.isCurrent || step.isDone || step.hasProblem ? 0 : 1
                        border.color: Theme.outlineStrong

                        Icon {
                            anchors.centerIn: parent
                            name: step.hasProblem ? "alert" : "check"
                            size: 14
                            color: step.hasProblem ? Theme.toneText("error") : step.isCurrent ? Theme.textOnAccent : Theme.toneText("success")
                            visible: step.hasProblem || (step.isDone && !step.isCurrent)
                        }
                        Text {
                            anchors.centerIn: parent
                            text: step.index + 1
                            color: step.isCurrent ? Theme.textOnAccent : Theme.textSecondary
                            font.pixelSize: Theme.fontSmall
                            font.weight: Font.DemiBold
                            visible: !step.hasProblem && (step.isCurrent || !step.isDone)
                        }
                    }
                    Label {
                        text: step.modelData
                        color: step.isCurrent ? Theme.text : Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                        font.weight: step.isCurrent ? Font.DemiBold : Font.Normal
                    }
                }
            }
        }
    }
    Item {
        Layout.fillWidth: true
    }
}
