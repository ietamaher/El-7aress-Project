import QtQuick
import QtQuick.Shapes

/**
 * Tracking Box Visualization
 * Shows the bounding box around tracked targets
 */
Rectangle {
    id: root

    // Properties
    property color boxColor: "yellow"
    property bool dashed: false
    property real cornerLength: 15

    color: "transparent"
    border.width: 0  // We'll draw custom corners instead

    // Corner lines (more visible than full box)
    // Top-left corner
    Shape {
        anchors.left: parent.left
        anchors.top: parent.top
        width: cornerLength
        height: cornerLength

        ShapePath {
            strokeWidth: 3
            strokeColor: "#0A0A0A"
            capStyle: ShapePath.RoundCap

            PathMove { x: 0; y: cornerLength }
            PathLine { x: 0; y: 0 }
            PathLine { x: cornerLength; y: 0 }
        }

        ShapePath {
            strokeWidth: 2
            strokeColor: root.boxColor
            strokeStyle: root.dashed ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [4, 4]
            capStyle: ShapePath.RoundCap

            PathMove { x: 0; y: cornerLength }
            PathLine { x: 0; y: 0 }
            PathLine { x: cornerLength; y: 0 }
        }
    }

    // Top-right corner
    Shape {
        anchors.right: parent.right
        anchors.top: parent.top
        width: cornerLength
        height: cornerLength

        ShapePath {
            strokeWidth: 3
            strokeColor: "#0A0A0A"
            capStyle: ShapePath.RoundCap

            PathMove { x: cornerLength; y: cornerLength }
            PathLine { x: cornerLength; y: 0 }
            PathLine { x: 0; y: 0 }
        }

        ShapePath {
            strokeWidth: 2
            strokeColor: root.boxColor
            strokeStyle: root.dashed ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [4, 4]
            capStyle: ShapePath.RoundCap

            PathMove { x: cornerLength; y: cornerLength }
            PathLine { x: cornerLength; y: 0 }
            PathLine { x: 0; y: 0 }
        }
    }

    // Bottom-left corner
    Shape {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: cornerLength
        height: cornerLength

        ShapePath {
            strokeWidth: 3
            strokeColor: "#0A0A0A"
            capStyle: ShapePath.RoundCap

            PathMove { x: 0; y: 0 }
            PathLine { x: 0; y: cornerLength }
            PathLine { x: cornerLength; y: cornerLength }
        }

        ShapePath {
            strokeWidth: 2
            strokeColor: root.boxColor
            strokeStyle: root.dashed ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [4, 4]
            capStyle: ShapePath.RoundCap

            PathMove { x: 0; y: 0 }
            PathLine { x: 0; y: cornerLength }
            PathLine { x: cornerLength; y: cornerLength }
        }
    }

    // Bottom-right corner
    Shape {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: cornerLength
        height: cornerLength

        ShapePath {
            strokeWidth: 3
            strokeColor: "#0A0A0A"
            capStyle: ShapePath.RoundCap

            PathMove { x: cornerLength; y: 0 }
            PathLine { x: cornerLength; y: cornerLength }
            PathLine { x: 0; y: cornerLength }
        }

        ShapePath {
            strokeWidth: 2
            strokeColor: root.boxColor
            strokeStyle: root.dashed ? ShapePath.DashLine : ShapePath.SolidLine
            dashPattern: [4, 4]
            capStyle: ShapePath.RoundCap

            PathMove { x: cornerLength; y: 0 }
            PathLine { x: cornerLength; y: cornerLength }
            PathLine { x: 0; y: cornerLength }
        }
    }

    // Optional: Full box outline (commented out for performance, use corners only)
    /*
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        color: "transparent"
        border.width: 2
        border.color: root.boxColor
    }
    */
}
