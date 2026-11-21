import QtQuick
import QtQuick.Controls.Material

// The style's outlined text area with an outline that keeps 3:1 against the surface (see AppTextField), and
// with Tab leaving the field.
TextArea {
    id: control

    Material.containerStyle: Material.Outlined
    placeholderTextColor: Theme.textSecondary
    selectByMouse: true
    // A text area keeps Tab for itself and types a tab character, which leaves a keyboard user stuck in it.
    // None of the texts here needs tab characters, so Tab moves on as everywhere else.
    Keys.onTabPressed: control.nextItemInFocusChain(true).forceActiveFocus(Qt.TabFocusReason)
    Keys.onBacktabPressed: control.nextItemInFocusChain(false).forceActiveFocus(Qt.BacktabFocusReason)

    Binding {
        target: control.background
        property: "outlineColor"
        value: control.enabled && control.hovered ? Theme.text : Theme.outlineStrong
    }
}
