pragma Singleton

import QtQuick

// How many dialogs are open. While one is, the shortcuts of the page behind it rest and brief messages wait.
QtObject {
    property int count: 0
    readonly property bool any: count > 0
}
