import QtQuick
import QtQuick.Controls

/**
 * SubMenu - Reusable submenu component for Reticle, Color, etc.
 *
 * Generic submenu widget that can display any list of options
 * Shows live preview as user navigates
 */
Rectangle {
    id: submenuRoot
    width: 350
    height: 350
    color: "#CC000000"
    border.color: menuViewModel ? menuViewModel.accentColor : "#46E2A5"
    border.width: 2
    radius: 4

    property var viewModel: menuViewModel // From context property
    property string menuType: "reticle" // "reticle" or "color"

    // Position in top-left area
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.leftMargin: 10
    anchors.topMargin: 150

    visible: {
        if (!viewModel) return false
        if (menuType === "reticle") return viewModel.reticleMenuVisible
        if (menuType === "color") return viewModel.colorMenuVisible
        return false
    }

    z: 100

    // ========================================================================
    // HEADER SECTION
    // ========================================================================
    Rectangle {
        id: headerSection
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "#DD000000"
        border.color: viewModel ? viewModel.accentColor : "#46E2A5"
        border.width: 1
        radius: 4

        Column {
            anchors.centerIn: parent
            spacing: 4

            Text {
                text: viewModel ? viewModel.menuTitle : "Submenu"
                font.pixelSize: 16
                font.bold: true
                font.family: "Archivo Narrow"
                color: viewModel ? viewModel.accentColor : "#46E2A5"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: viewModel ? viewModel.menuDescription : ""
                font.pixelSize: 11
                font.family: "Archivo Narrow"
                color: "#AAAAAA"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    // ========================================================================
    // MENU OPTIONS LIST
    // ========================================================================
    ListView {
        id: submenuListView
        anchors.top: headerSection.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footerSection.top
        anchors.margins: 8
        clip: true

        model: viewModel ? viewModel.menuOptions : []
        currentIndex: viewModel ? viewModel.currentSelection : 0

        // Scroll to keep current item visible
        onCurrentIndexChanged: {
            positionViewAtIndex(currentIndex, ListView.Contain)
        }

        delegate: Rectangle {
            width: submenuListView.width
            height: 32

            property bool isSelected: index === submenuListView.currentIndex

            color: isSelected ? (viewModel ? viewModel.accentColor : "#46E2A5") : "transparent"
            border.color: isSelected ? "white" : "transparent"
            border.width: isSelected ? 1 : 0
            radius: 2

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: modelData
                font.pixelSize: 14
                font.bold: isSelected
                font.family: "Archivo Narrow"
                color: isSelected ? "#000000" : (viewModel ? viewModel.accentColor : "#46E2A5")
            }

            // Selection indicator arrow
            Text {
                visible: isSelected
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: "►"
                font.pixelSize: 14
                font.bold: true
                color: "#000000"
            }
        }
    }

    // ========================================================================
    // FOOTER SECTION (Navigation Hints)
    // ========================================================================
    Rectangle {
        id: footerSection
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 45
        color: "#DD000000"
        border.color: viewModel ? viewModel.accentColor : "#46E2A5"
        border.width: 1
        radius: 4

        Column {
            anchors.centerIn: parent
            spacing: 2

            Text {
                text: "↑ UP  |  ↓ DOWN  |  ● SELECT"
                font.pixelSize: 11
                font.family: "Archivo Narrow"
                color: "#AAAAAA"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Live Preview Enabled"
                font.pixelSize: 9
                font.family: "Archivo Narrow"
                color: "#777777"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    // ========================================================================
    // AUTO-CLOSE TIMER (15 seconds)
    // ========================================================================
    Timer {
        id: autoCloseTimer
        interval: 15000 // 15 seconds
        running: submenuRoot.visible
        repeat: false
        onTriggered: {
            if (viewModel) {
                viewModel.closeCurrentMenu()
            }
        }
    }

    // Reset timer on any interaction
    Connections {
        target: viewModel
        function onCurrentSelectionChanged() {
            if (autoCloseTimer.running) {
                autoCloseTimer.restart()
            }
        }
    }
}
