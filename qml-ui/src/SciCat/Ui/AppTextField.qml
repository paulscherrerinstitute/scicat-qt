import QtQuick
import QtQuick.Controls.Material

// The style's outlined text field with an outline and a placeholder that can be seen: Material's own are 2.6:1 on a
// white surface, and the edge of a control needs 3:1.
TextField {
    id: control

    Material.containerStyle: Material.Outlined
    placeholderTextColor: Theme.textSecondary
    selectByMouse: true

    Binding {
        target: control.background
        property: "outlineColor"
        value: control.enabled && control.hovered ? Theme.text : Theme.outlineStrong
    }
}
