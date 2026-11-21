import QtQuick
import QtQuick.Shapes

// A themable line icon. Decorative by default: give the control that holds it an accessible name.
Item {
    id: root

    property string name
    property color color: Theme.text
    property real size: 20

    implicitWidth: size
    implicitHeight: size
    Accessible.ignored: true

    Shape {
        anchors.centerIn: parent
        width: 24
        height: 24
        scale: root.size / 24
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: root.color
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg {
                path: Icons.path(root.name)
            }
        }
    }
}
