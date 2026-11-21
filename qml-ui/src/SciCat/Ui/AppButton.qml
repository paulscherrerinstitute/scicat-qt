import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Templates as T

// The application's button: one filled primary action per area, the rest tonal or plain text.
// Drawn from plain rectangles, so it looks the same with every Qt Quick renderer.
T.Button {
    id: control

    enum Variant { Filled, Tonal, Plain, Danger }

    property int variant: AppButton.Tonal
    property string iconName
    property bool busy: false

    readonly property bool solid: variant === AppButton.Filled || variant === AppButton.Danger
    readonly property color base: variant === AppButton.Danger ? Theme.danger : Theme.accent
    readonly property color tint: !enabled ? Theme.textDisabled : solid ? Theme.textOnAccent
        : variant === AppButton.Plain ? Theme.link : Theme.text

    implicitWidth: Math.max(implicitBackgroundWidth, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight + topPadding + bottomPadding)
    leftPadding: variant === AppButton.Plain ? 10 : 14
    rightPadding: variant === AppButton.Plain ? 10 : 14
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true
    font.pixelSize: Theme.fontBody
    font.weight: Font.Medium
    Accessible.name: text

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 36
        radius: Theme.radiusSmall
        color: !control.enabled ? (control.solid || control.variant === AppButton.Tonal ? Theme.hover : "transparent")
            : control.solid ? (control.down ? Qt.darker(control.base, 1.25) : control.hovered ? Qt.darker(control.base, 1.1) : control.base)
            : control.down ? Theme.selected : control.hovered ? Theme.hover
            : control.variant === AppButton.Tonal ? Theme.surface : "transparent"
        border.width: control.variant === AppButton.Tonal ? 1 : 0
        border.color: control.enabled ? Theme.outlineStrong : Theme.outline

        FocusRing {
            shown: control.visualFocus
        }
    }
    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        RowLayout {
            id: row

            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width)
            spacing: 6

            BusyIndicator {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                padding: 0
                // in the colour of the label: the style's own colour is the one a filled button is painted with
                Material.accent: control.tint
                visible: control.busy
                running: control.busy && control.visible
            }
            Icon {
                name: control.iconName
                size: 18
                color: control.tint
                visible: control.iconName !== "" && !control.busy
            }
            Label {
                Layout.fillWidth: true
                text: control.text
                font: control.font
                color: control.tint
                elide: Text.ElideRight
                visible: text !== ""
            }
        }
    }
}
