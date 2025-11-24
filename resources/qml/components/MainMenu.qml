import QtQuick
import QtQuick.Controls

/**
 * MainMenu - Professional menu widget for 3-button navigation
 *
 * Displays menu options with current selection highlighted
 * Compatible with UP, DOWN, MENU/VAL button navigation
 */
Rectangle {
    id: menuRoot
    width: 400
    height: 550
    color: "#CC000000"
    border.color: menuViewModel ? menuViewModel.accentColor : "#46E2A5"
    border.width: 2
    radius: 4

    property var viewModel: menuViewModel // From context property

    // Position in top-left area
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.leftMargin: 10
    anchors.topMargin: 150

    visible: viewModel ? viewModel.mainMenuVisible : false
    z: 100

    // ========================================================================
    // HEADER SECTION
    // ========================================================================
    Rectangle {
        id: headerSection
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 70
        color: "#DD000000"
        border.color: viewModel ? viewModel.accentColor : "#46E2A5"
        border.width: 1
        radius: 4

        Column {
            anchors.centerIn: parent
            spacing: 4

            Text {
                text: viewModel ? viewModel.menuTitle : "Main Menu"
                font.pixelSize: 18
                font.bold: true
                font.family: "Archivo Narrow"
                color: viewModel ? viewModel.accentColor : "#46E2A5"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: viewModel ? viewModel.menuDescription : "Navigate Through Options"
                font.pixelSize: 12
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
        id: menuListView
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

        delegate: Item {
            width: menuListView.width
            height: isSectionHeader ? 35 : 32

            property bool isSectionHeader: modelData.startsWith("---")
            property bool isSelected: index === menuListView.currentIndex

            // Section header rendering
            Rectangle {
                visible: isSectionHeader
                anchors.fill: parent
                anchors.margins: 2
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Archivo Narrow"
                    color: "#888888"
                }
            }

            // Regular menu item rendering
            Rectangle {
                visible: !isSectionHeader
                anchors.fill: parent
                anchors.margins: 1
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

        // Scrollbar
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
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
        height: 50
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
                text: "Press MENU to select"
                font.pixelSize: 10
                font.family: "Archivo Narrow"
                color: "#777777"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    // ========================================================================
    // AUTO-CLOSE TIMER (20 seconds)
    // ========================================================================
    Timer {
        id: autoCloseTimer
        interval: 20000 // 20 seconds
        running: menuRoot.visible
        repeat: false
        onTriggered: {
            if (viewModel) {
                viewModel.closeCurrentMenu()
            }
        }
    }

    // Reset timer on any interaction (handled by navigation)
    Connections {
        target: viewModel
        function onCurrentSelectionChanged() {
            if (autoCloseTimer.running) {
                autoCloseTimer.restart()
            }
        }
    }
}
