import QtQuick
import QtQuick.Shapes

/**
 * CCIP (Continuously Computed Impact Point) Pipper
 * Shows where projectiles will land based on ballistics calculation
 */
Item {
    id: root
    width: 60
    height: 60

    // Properties
    property bool pipperEnabled: false
    property string status: "Off"  // "Off", "Computing", "Ready", "LAG", "ZOOM"
    property color accentColor: "#46E2A5"

    visible: pipperEnabled && (status !== "Off")

    // Visual representation
    Shape {
        anchors.centerIn: parent
        visible: root.visible

        // Outer circle (outline)
        ShapePath {
            strokeWidth: 3
            strokeColor: "#0A0A0A"
            fillColor: "transparent"

            PathAngleArc {
                centerX: 30
                centerY: 30
                radiusX: 20
                radiusY: 20
                startAngle: 0
                sweepAngle: 360
            }
        }

        // Inner circle (main)
        ShapePath {
            strokeWidth: 2
            strokeColor: getCcipColor()
            fillColor: "transparent"

            PathAngleArc {
                centerX: 30
                centerY: 30
                radiusX: 20
                radiusY: 20
                startAngle: 0
                sweepAngle: 360
            }
        }

        // Center dot
        ShapePath {
            fillColor: getCcipColor()
            strokeColor: "transparent"

            PathAngleArc {
                centerX: 30
                centerY: 30
                radiusX: 3
                radiusY: 3
                startAngle: 0
                sweepAngle: 360
            }
        }

        // Crosshair
        ShapePath {
            strokeWidth: 2
            strokeColor: getCcipColor()
            capStyle: ShapePath.RoundCap

            // Horizontal
            PathMove { x: 10; y: 30 }
            PathLine { x: 22; y: 30 }
            PathMove { x: 38; y: 30 }
            PathLine { x: 50; y: 30 }

            // Vertical
            PathMove { x: 30; y: 10 }
            PathLine { x: 30; y: 22 }
            PathMove { x: 30; y: 38 }
            PathLine { x: 30; y: 50 }
        }
    }

    // Status indicator (optional text label)
    Text {
        visible: status === "LAG" || status === "ZOOM"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.bottom
        anchors.topMargin: 5
        text: status
        font.pixelSize: 12
        font.bold: true
        font.family: "Archivo Narrow"
        color: status === "LAG" ? "yellow" : "#C81428"
        style: Text.Outline
        styleColor: "black"
    }

    // Pulsing animation when computing
    SequentialAnimation on opacity {
        running: status === "Computing"
        loops: Animation.Infinite

        NumberAnimation { from: 1.0; to: 0.5; duration: 500 }
        NumberAnimation { from: 0.5; to: 1.0; duration: 500 }
    }

    // Helper function to determine color based on status
    function getCcipColor() {
        switch (status) {
            case "Ready":
                return accentColor  // Green
            case "Computing":
                return "yellow"
            case "LAG":
                return "yellow"
            case "ZOOM":
                return "#C81428"  // Red
            default:
                return accentColor
        }
    }
}
