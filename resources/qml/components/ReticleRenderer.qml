import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 200
    height: 200

    // Properties
    property int reticleType: 1  // 0=Basic, 1=BoxCrosshair, 2=Standard, 3=Precision, 4=MilDot
    property color color: "#46E2A5"
    property real currentFov: 45.0
    property bool lacActive: false
    property real rangeMeters: 0
    property real confidenceLevel: 1.0

    // Internal constants
    readonly property real pixelsPerMil: calculatePixelsPerMil()

    // Calculate pixels per mil based on FOV
    function calculatePixelsPerMil() {
        var screenWidthPixels = 1024;
        var hfovDegrees = currentFov;
        var hfovMils = hfovDegrees * 17.777777; // 1 degree = 17.777777 mils
        return screenWidthPixels / hfovMils;
    }

    // Dynamic loader for reticle types
    Loader {
        anchors.centerIn: parent
        sourceComponent: {
            switch (reticleType) {
                case 0: return basicReticle
                case 1: return boxCrosshairReticle
                case 2: return standardCrosshairReticle
                case 3: return precisionCrosshairReticle
                case 4: return milDotReticle
                default: return basicReticle
            }
        }
    }

    // ========================================================================
    // RETICLE TYPE 0: BASIC (Simple Cross)
    // ========================================================================
    Component {
        id: basicReticle
        Shape {
            width: 40
            height: 40

            ShapePath {
                strokeWidth: 2
                strokeColor: root.color
                strokeStyle: ShapePath.SolidLine
                capStyle: ShapePath.RoundCap

                // Horizontal line
                PathMove { x: 0; y: 20 }
                PathLine { x: 40; y: 20 }

                // Vertical line
                PathMove { x: 20; y: 0 }
                PathLine { x: 20; y: 40 }
            }

            // Outline (for contrast)
            ShapePath {
                strokeWidth: 4
                strokeColor: "#0A0A0A"
                strokeStyle: ShapePath.SolidLine
                capStyle: ShapePath.RoundCap

                PathMove { x: 0; y: 20 }
                PathLine { x: 40; y: 20 }
                PathMove { x: 20; y: 0 }
                PathLine { x: 20; y: 40 }
            }
        }
    }

    // ========================================================================
    // RETICLE TYPE 1: BOX CROSSHAIR (Military Standard)
    // ========================================================================
    Component {
        id: boxCrosshairReticle
        Item {
            width: 180
            height: 180

            Shape {
                anchors.centerIn: parent

                // Outline (dark)
                ShapePath {
                    strokeWidth: 4
                    strokeColor: "#0A0A0A"
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap

                    // Center box
                    //PathRectangle { x: -25; y: -25; width: 50; height: 50 }

                    // Crosshair lines with gaps
                    // Left line
                    PathMove { x: -80; y: 0 }
                    PathLine { x: -27; y: 0 }
                    // Right line
                    PathMove { x: 27; y: 0 }
                    PathLine { x: 80; y: 0 }
                    // Top line
                    PathMove { x: 0; y: -80 }
                    PathLine { x: 0; y: -27 }
                    // Bottom line
                    PathMove { x: 0; y: 27 }
                    PathLine { x: 0; y: 80 }
                }

                // Main reticle (colored)
                ShapePath {
                    strokeWidth: 2
                    strokeColor: root.color
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap

                    //PathRectangle { x: -25; y: -25; width: 50; height: 50 }

                    PathMove { x: -80; y: 0 }
                    PathLine { x: -27; y: 0 }
                    PathMove { x: 27; y: 0 }
                    PathLine { x: 80; y: 0 }
                    PathMove { x: 0; y: -80 }
                    PathLine { x: 0; y: -27 }
                    PathMove { x: 0; y: 27 }
                    PathLine { x: 0; y: 80 }
                }
            }
        }
    }

    // ========================================================================
    // RETICLE TYPE 2: STANDARD CROSSHAIR
    // ========================================================================
    Component {
        id: standardCrosshairReticle
        Shape {
            width: 120
            height: 120

            // Outline
            ShapePath {
                strokeWidth: 4
                strokeColor: "#0A0A0A"
                capStyle: ShapePath.RoundCap

                PathMove { x: 10; y: 60 }
                PathLine { x: 50; y: 60 }
                PathMove { x: 70; y: 60 }
                PathLine { x: 110; y: 60 }

                PathMove { x: 60; y: 10 }
                PathLine { x: 60; y: 50 }
                PathMove { x: 60; y: 70 }
                PathLine { x: 60; y: 110 }
            }

            // Main
            ShapePath {
                strokeWidth: 2
                strokeColor: root.color
                capStyle: ShapePath.RoundCap

                PathMove { x: 10; y: 60 }
                PathLine { x: 50; y: 60 }
                PathMove { x: 70; y: 60 }
                PathLine { x: 110; y: 60 }

                PathMove { x: 60; y: 10 }
                PathLine { x: 60; y: 50 }
                PathMove { x: 60; y: 70 }
                PathLine { x: 60; y: 110 }
            }
        }
    }

    // ========================================================================
    // RETICLE TYPE 3: PRECISION CROSSHAIR (with range marks)
    // ========================================================================
    Component {
        id: precisionCrosshairReticle
        Item {
            width: 200
            height: 200

            Shape {
                anchors.centerIn: parent

                // Outline
                ShapePath {
                    strokeWidth: 4
                    strokeColor: "#0A0A0A"
                    capStyle: ShapePath.RoundCap

                    // Main cross
                    PathMove { x: -100; y: 0 }
                    PathLine { x: 100; y: 0 }
                    PathMove { x: 0; y: -100 }
                    PathLine { x: 0; y: 100 }

                    // Tick marks every 15 pixels (represents distance)
                    /*for (var i = 1; i <= 5; i++) {
                        var offset = i * 15;
                        // Right ticks
                        PathMove { x: offset; y: -5 }
                        PathLine { x: offset; y: 5 }
                        // Left ticks
                        PathMove { x: -offset; y: -5 }
                        PathLine { x: -offset; y: 5 }
                        // Bottom ticks
                        PathMove { x: -5; y: offset }
                        PathLine { x: 5; y: offset }
                        // Top ticks
                        PathMove { x: -5; y: -offset }
                        PathLine { x: 5; y: -offset }
                    }*/
                }

                // Main
                ShapePath {
                    strokeWidth: 2
                    strokeColor: root.color
                    capStyle: ShapePath.RoundCap

                    PathMove { x: -100; y: 0 }
                    PathLine { x: 100; y: 0 }
                    PathMove { x: 0; y: -100 }
                    PathLine { x: 0; y: 100 }

                    /*for (var i = 1; i <= 5; i++) {
                        var offset = i * 15;
                        PathMove { x: offset; y: -5 }
                        PathLine { x: offset; y: 5 }
                        PathMove { x: -offset; y: -5 }
                        PathLine { x: -offset; y: 5 }
                        PathMove { x: -5; y: offset }
                        PathLine { x: 5; y: offset }
                        PathMove { x: -5; y: -offset }
                        PathLine { x: 5; y: -offset }
                    }*/
                }
            }

            // Center dot
            Rectangle {
                width: 4
                height: 4
                radius: 2
                color: root.color
                anchors.centerIn: parent
            }
        }
    }

    // ========================================================================
    // RETICLE TYPE 4: MIL-DOT (Sniper-style with mil dots)
    // ========================================================================
    Component {
        id: milDotReticle
        Item {
            width: 240
            height: 240

            Shape {
                anchors.centerIn: parent

                // Outline
                ShapePath {
                    strokeWidth: 4
                    strokeColor: "#0A0A0A"
                    capStyle: ShapePath.RoundCap

                    PathMove { x: -120; y: 0 }
                    PathLine { x: 120; y: 0 }
                    PathMove { x: 0; y: -120 }
                    PathLine { x: 0; y: 120 }
                }

                // Main crosshair
                ShapePath {
                    strokeWidth: 2
                    strokeColor: root.color
                    capStyle: ShapePath.RoundCap

                    PathMove { x: -120; y: 0 }
                    PathLine { x: 120; y: 0 }
                    PathMove { x: 0; y: -120 }
                    PathLine { x: 0; y: 120 }
                }
            }

            // Mil dots (assuming 1 mil = pixelsPerMil)
            Repeater {
                model: 8 // 4 dots per direction
                Rectangle {
                    property int dotIndex: index + 1
                    width: 3
                    height: 3
                    radius: 1.5
                    color: root.color
                    x: (dotIndex * 20) - 1.5 + 120 // Offset to center, then right
                    y: 120 - 1.5 // Centered vertically
                }
            }

            Repeater {
                model: 8
                Rectangle {
                    property int dotIndex: index + 1
                    width: 3
                    height: 3
                    radius: 1.5
                    color: root.color
                    x: 120 - (dotIndex * 20) - 1.5 // Left side
                    y: 120 - 1.5
                }
            }

            Repeater {
                model: 8
                Rectangle {
                    property int dotIndex: index + 1
                    width: 3
                    height: 3
                    radius: 1.5
                    color: root.color
                    x: 120 - 1.5
                    y: (dotIndex * 20) - 1.5 + 120 // Bottom
                }
            }

            Repeater {
                model: 8
                Rectangle {
                    property int dotIndex: index + 1
                    width: 3
                    height: 3
                    radius: 1.5
                    color: root.color
                    x: 120 - 1.5
                    y: 120 - (dotIndex * 20) - 1.5 // Top
                }
            }
        }
    }
}
