pragma ComponentBehavior: Bound

import QtQuick

// Placeholder rows in the shape of the list that is loading.
Column {
    id: root

    property int rows: 8
    property int rowHeight: Theme.rowHeight

    spacing: 0
    Accessible.ignored: true

    Repeater {
        model: root.rows

        Item {
            id: row

            required property int index

            width: root.width
            height: root.rowHeight

            Rectangle {
                x: Theme.gapL
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(parent.width * (0.25 + (row.index * 37 % 30) / 100), parent.width - 2 * Theme.gapL)
                height: 12
                radius: 6
                color: Theme.hover
            }
            Rectangle {
                anchors.right: parent.right
                anchors.rightMargin: Theme.gapL
                anchors.verticalCenter: parent.verticalCenter
                width: 90
                height: 12
                radius: 6
                color: Theme.hover
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.outline
            }
        }
    }

    SequentialAnimation on opacity {
        running: root.visible
        loops: Animation.Infinite

        OpacityAnimator {
            from: 1
            to: 0.45
            duration: 700
        }
        OpacityAnimator {
            from: 0.45
            to: 1
            duration: 700
        }
    }
}
