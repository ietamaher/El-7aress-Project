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

    // ========================================================================
    // MENU SYSTEM (z: 100+)
    // ========================================================================

    // Main Menu
    MainMenu {
        id: mainMenu
        anchors.fill: parent
    }

    // Reticle Submenu
    SubMenu {
        id: reticleMenu
        menuType: "reticle"
        anchors.fill: parent
    }

    // Color Submenu
    SubMenu {
        id: colorMenu
        menuType: "color"
        anchors.fill: parent
    }

    // Brightness Overlay
    SimpleOverlay {
        id: brightnessOverlay
        overlayType: "brightness"
    }

    // About Dialog
    SimpleOverlay {
        id: aboutDialog
        overlayType: "about"
    }

    // NOTE: Specialized overlays (Zeroing, Windage, Zone Definition, System Status)
    // can remain as Qt Widgets or be ported to QML later
    // They are shown/hidden via menuViewModel state management
}
