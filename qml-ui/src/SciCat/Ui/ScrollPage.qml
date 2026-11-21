import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

// A page that scrolls and brings the control that takes keyboard focus into view.
// Like every ScrollView it sizes its content from its one visual child: give it exactly one.
ScrollView {
    id: view

    // the control that took the focus a moment ago, while the page is still arranging itself around it
    property Item settling: null
    // the control the page last followed: coming back to the window must not scroll back to it
    property Item followed: null

    function reveal(item: Item) {
        const flick = view.contentItem as Flickable
        if (!item || !flick)
            return
        // room for the label above a field, and for the line of help or of a problem below it
        const above = 28
        const below = 52
        const top = item.mapToItem(flick.contentItem, 0, 0).y - above
        const bottom = top + above + item.height + below
        const last = Math.max(0, flick.contentHeight - flick.height)
        if (top < flick.contentY || bottom - top > flick.height)
            flick.contentY = Math.max(0, Math.min(top, last))
        else if (bottom > flick.contentY + flick.height)
            flick.contentY = Math.min(last, bottom - flick.height)
    }

    function scrollToTop() {
        const flick = view.contentItem as Flickable
        if (flick)
            flick.contentY = 0
    }

    // How an item got the focus. The controls and the text fields say so, a plain item does not. Asked of the
    // types they all share, whatever style draws them.
    function focusReasonOf(item: Item): int {
        const control = item as T.Control
        if (control)
            return control.focusReason
        const field = item as T.TextField
        if (field)
            return field.focusReason
        const area = item as T.TextArea
        return area ? area.focusReason : Qt.OtherFocusReason
    }

    function holds(item: Item): bool {
        for (let above = item; above; above = above.parent) {
            if (above === view)
                return true
        }
        return false
    }

    // Scrolls to an item, and keeps to it for a moment while the page is still arranging itself around it.
    function bringIntoView(item: Item) {
        view.settling = item
        settleTimer.restart()
        view.reveal(item)
    }

    // For a focus the page set itself, such as the first field with a problem: always, also when that field had
    // the focus before and the page was scrolled away from it.
    function revealFocused() {
        const focused = view.Window.activeFocusItem
        if (!view.holds(focused))
            return
        view.followed = focused
        view.bringIntoView(focused)
    }

    // For a focus that moved by itself: by Tab, by a shortcut, by the page that opened.
    function followFocus() {
        const focused = view.Window.activeFocusItem
        if (!view.visible || !focused || focused === view.followed)
            return
        view.followed = focused
        // A click puts the focus where the pointer already is. Moving the page under it would make the click,
        // or the next one, land somewhere else.
        if (view.focusReasonOf(focused) === Qt.MouseFocusReason)
            return
        view.revealFocused()
    }

    contentWidth: availableWidth
    // after the change has gone through: the reason of the focus is known by then
    Window.onActiveFocusItemChanged: Qt.callLater(view.followFocus)

    Timer {
        id: settleTimer

        interval: 300
        onTriggered: view.settling = null
    }
    Connections {
        target: view.contentItem

        // A new row or a message may still be growing the page under the control that just took the focus.
        // Later changes of the height leave the view alone: by then the user may have scrolled elsewhere.
        function onContentHeightChanged() {
            if (view.settling)
                view.reveal(view.settling)
        }
    }
}
