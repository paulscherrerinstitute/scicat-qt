import QtQuick

// A text field for a value that is kept somewhere else. It shows the kept value, except while it is being typed
// in, and hands the typed text over when the editing ends: with Return, on leaving the field, when the field is
// hidden, or when commit() is called because the value is needed now.
AppTextField {
    id: control

    property string stored

    signal committed(string text)

    function commit() {
        if (text !== stored)
            committed(text)
        // what the keeper made of it: trimmed, completed, or replaced by a default
        text = stored
    }

    onStoredChanged: {
        if (!activeFocus)
            text = stored
    }
    onEditingFinished: commit()
    onVisibleChanged: {
        if (!visible && text !== stored)
            commit()
    }
    Component.onCompleted: text = stored
}
