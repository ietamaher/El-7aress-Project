import QtQuick
import QtQuick.Controls
import "qrc:/qml/components"
import "components"

Rectangle {
    id: mainWindow
    width: 1024
    height: 768
    color: "black"

    // Video feed
    Image {
        id: videoDisplay
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "image://video/camera"
        cache: false

        Timer {
            interval: 33  // ~30 FPS
            running: true
            repeat: true
            onTriggered: {
                videoDisplay.source = "image://video/camera?" + Date.now()
            }
        }
    }

    // OSD Overlay
    OsdOverlay {
        id: osdOverlay
        anchors.fill: parent
        z: 10
    }
}
