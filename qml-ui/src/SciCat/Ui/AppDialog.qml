import QtQuick
import QtQuick.Controls.Material
import QtQuick.Templates as T

// A question or a small task in front of the window. Its title wraps, the keyboard focus starts on the control it
// names every time it opens, and it goes away by itself once what it asks about is over.
Dialog {
    id: dialog

    // takes the keyboard focus each time the dialog opens; for a question, the answer that changes nothing
    property Item initialFocusItem: null
    // false once the question has answered itself, such as "Stop the run?" after the run has ended
    property bool relevant: true
    // opened with the keyboard: then the focus is shown from the start
    property bool keyboardOpened: false

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    focus: true
    // not by a click next to it: that is too easy to do by accident with a list of ticked rows in it
    closePolicy: Popup.CloseOnEscape

    // The title is as wide as the dialog and takes the lines it needs. It sits in an item of its own so that its
    // length says nothing about how wide the dialog should be: the dialog gives the width, the title the height.
    header: Item {
        implicitHeight: titleLabel.implicitHeight
        visible: dialog.title !== ""

        Label {
            id: titleLabel

            anchors.left: parent.left
            anchors.right: parent.right
            text: dialog.title
            color: Theme.text
            font.pixelSize: Theme.fontHeading
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            leftPadding: 24
            rightPadding: 24
            topPadding: 24
            Accessible.role: Accessible.Heading
        }
    }

    onAboutToShow: {
        const overlay = dialog.parent as Item
        // the type all controls share, whatever style draws them
        const opener = (overlay ? overlay.Window.activeFocusItem : null) as T.Control
        keyboardOpened = opener !== null && opener.visualFocus
        // the focus inside the dialog is decided before it is shown, so a key pressed at once meets the safe answer
        if (initialFocusItem)
            initialFocusItem.focus = true
    }
    // The control has the focus by now, "because a dialog opened". After a key press that is said to be a focus
    // reached with the keyboard, which is what makes a control show its focus.
    onActiveFocusChanged: {
        const control = initialFocusItem as T.Control
        if (activeFocus && keyboardOpened && control && control.activeFocus)
            control.focusReason = Qt.TabFocusReason
    }
    onVisibleChanged: Modals.count += visible ? 1 : -1
    onRelevantChanged: {
        if (!relevant)
            close()
    }
    // a page can be taken down while its dialog is still closing, on signing out for one
    Component.onDestruction: {
        if (visible)
            Modals.count -= 1
    }
}
