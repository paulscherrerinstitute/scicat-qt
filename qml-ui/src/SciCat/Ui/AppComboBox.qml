import QtQuick
import QtQuick.Controls.Material

// The style's combo box with an outline that keeps 3:1 against the surface, see AppTextField.
ComboBox {
    id: control

    Binding {
        target: control.background
        property: "outlineColor"
        value: control.enabled && control.hovered ? Theme.text : Theme.outlineStrong
    }
}
