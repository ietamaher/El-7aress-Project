import QtQuick

Item {
    id: root
    property color boxColor: "yellow"
    property color outlineColor: "#000000"
    property real outlineWidth: 4
    property bool dashed: false
    readonly property real cornerLength: 15

    // Eight corner outlines (behind main corners)
    Repeater {
        model: 8
        delegate: Rectangle {
            property bool isHorizontal: index % 2 === 0
            property int corner: Math.floor(index / 2) // 0=TL, 1=TR, 2=BL, 3=BR

            width: isHorizontal ? cornerLength : 2 + root.outlineWidth
            height: isHorizontal ? 2 + root.outlineWidth : cornerLength
            color: root.outlineColor

            x: {
                if (corner === 0 || corner === 2) return 0; // Left corners
                if (isHorizontal) return root.width - cornerLength; // Right horizontal
                return root.width - (2 + root.outlineWidth); // Right vertical
            }

            y: {
                if (corner === 0 || corner === 1) return 0; // Top corners
                if (isHorizontal) return root.height - (2 + root.outlineWidth); // Bottom horizontal
                return root.height - cornerLength; // Bottom vertical
            }

            opacity: root.dashed ? 0.7 : 1.0
        }
    }

    // Eight main corner lines (on top of outlines)
    Repeater {
        model: 8
        delegate: Rectangle {
            property bool isHorizontal: index % 2 === 0
            property int corner: Math.floor(index / 2) // 0=TL, 1=TR, 2=BL, 3=BR

            width: isHorizontal ? cornerLength : 2
            height: isHorizontal ? 2 : cornerLength
            color: root.boxColor

            x: {
                if (corner === 0 || corner === 2) return 0; // Left corners
                if (isHorizontal) return root.width - cornerLength; // Right horizontal
                return root.width - 2; // Right vertical
            }

            y: {
                if (corner === 0 || corner === 1) return 0; // Top corners
                if (isHorizontal) return root.height - 2; // Bottom horizontal
                return root.height - cornerLength; // Bottom vertical
            }

            opacity: root.dashed ? 0.8 : 1.0
        }
    }
}
