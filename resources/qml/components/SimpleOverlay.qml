import QtQuick

/**
 * SimpleOverlay - Generic overlay component for Brightness, About, etc.
 *
 * Displays a simple centered overlay with title and content
 */
Rectangle {
    id: overlayRoot
    width: 400
    height: 250
    color: "#DD000000"
    border.color: menuViewModel ? menuViewModel.accentColor : "#46E2A5"
    border.width: 2
    radius: 4

    property var viewModel: menuViewModel
    property string overlayType: "brightness" // "brightness", "about", "systemstatus"

    // Center on screen
    anchors.centerIn: parent

    visible: {
        if (!viewModel) return false
        if (overlayType === "brightness") return viewModel.brightnessMenuVisible
        if (overlayType === "about") return viewModel.aboutDialogVisible
        if (overlayType === "systemstatus") return viewModel.systemStatusOverlayVisible
        return false
    }

    z: 150

    // ========================================================================
    // HEADER
    // ========================================================================
    Text {
        id: titleText
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        text: viewModel ? viewModel.menuTitle : "Overlay"
        font.pixelSize: 18
        font.bold: true
        font.family: "Archivo Narrow"
        color: viewModel ? viewModel.accentColor : "#46E2A5"
    }

    // ========================================================================
    // CONTENT AREA (varies by overlay type)
    // ========================================================================
    Item {
        id: contentArea
        anchors.top: titleText.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: instructionsText.top
        anchors.bottomMargin: 20
        anchors.margins: 20

        // Brightness overlay content
        Column {
            visible: overlayType === "brightness"
            anchors.centerIn: parent
            spacing: 15

            Text {
                text: "Brightness: --%" // Updated from C++
                font.pixelSize: 24
                font.bold: true
                font.family: "Archivo Narrow"
                color: viewModel ? viewModel.accentColor : "#46E2A5"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Use UP/DOWN buttons\nto adjust brightness"
                font.pixelSize: 12
                font.family: "Archivo Narrow"
                color: "#AAAAAA"
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // About overlay content
        Column {
            visible: overlayType === "about"
            anchors.centerIn: parent
            spacing: 10

            Text {
                text: "RCWS Control System"
                font.pixelSize: 16
                font.bold: true
                font.family: "Archivo Narrow"
                color: viewModel ? viewModel.accentColor : "#46E2A5"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Version 1.0.0"
                font.pixelSize: 12
                font.family: "Archivo Narrow"
                color: "#AAAAAA"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "© 2025 Military Solutions"
                font.pixelSize: 11
                font.family: "Archivo Narrow"
                color: "#777777"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // System Status overlay - simplified (full widget handled separately)
        Text {
            visible: overlayType === "systemstatus"
            anchors.centerIn: parent
            text: "System Status\n(View device health)"
            font.pixelSize: 14
            font.family: "Archivo Narrow"
            color: viewModel ? viewModel.accentColor : "#46E2A5"
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // ========================================================================
    // INSTRUCTIONS
    // ========================================================================
    Text {
        id: instructionsText
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 15
        text: "Press MENU to close"
        font.pixelSize: 11
        font.family: "Archivo Narrow"
        color: "#777777"
    }
}
