import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    // Bind your aboutViewModel here (from C++ / JS). If null, use debugShow to test.
    property var viewModel: aboutViewModel

    // Debug toggle: set true to force-visible while wiring viewModel
    property bool debugShow: false

    // Accent color fallback (hex). If viewModel provides accentColor (string), it overrides.
    property color accentColor: viewModel && viewModel.accentColor ? viewModel.accentColor : "#46E2A5"

    // Visible only when either viewModel.visible is true OR debugShow = true
    visible: viewModel ? !!viewModel.visible : debugShow
    anchors.fill: parent

    // Semi-transparent black overlay (AARRGGBB)
    color: "#CC000000"

    Component.onCompleted: {
        console.log("About dialog created. visible=", root.visible, "accentColor=", accentColor)
    }

    // Clicking outside closes the dialog (if viewModel supplied)
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (viewModel) viewModel.visible = false
        }
    }

    // Main dialog (no rounding)
    Rectangle {
        id: aboutDialog
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 700)
        height: Math.min(parent.height * 0.85, 650)

        color: "#1A1A1A"       // dark panel background
        border.color: accentColor
        border.width: 1
        radius: 0              // NO rounding

        // prevent click-through: eat clicks on the dialog itself
        MouseArea {
            anchors.fill: parent
            onClicked: {} // eat clicks
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            // HEADER
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                // Logo rectangle (no rounding)
                Rectangle {
                    width: 80
                    height: 80
                    color: accentColor
                    radius: 0

                    Text {
                        anchors.centerIn: parent
                        text: "RCWS"
                        font.pixelSize: 24
                        font.bold: true
                        font.family: "Archivo Narrow"
                        color: "#0A0A0A"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: viewModel && viewModel.appName ? viewModel.appName : "El 7arress RCWS"
                        font.pixelSize: 28
                        font.bold: true
                        font.family: "Archivo Narrow"
                        color: accentColor
                    }

                    Text {
                        text: viewModel && viewModel.appVersion ? viewModel.appVersion : "Version 4.5"
                        font.pixelSize: 14
                        font.family: "Archivo Narrow"
                        color: "#AAAAAA"
                    }

                    Text {
                        text: viewModel && viewModel.buildDate ? viewModel.buildDate : ""
                        font.pixelSize: 12
                        font.family: "Archivo Narrow"
                        color: "#666666"
                    }
                }
            }

            // separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: accentColor
            }

            // SCROLLABLE CONTENT
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                // Use ColumnLayout inside ScrollView; width matches ScrollView's content area
                ColumnLayout {
                    id: scrollContent
                    width: parent.width
                    spacing: 18
                    anchors.margins: 4

                    // Credits Section
                    InfoSection {
                        title: "Credits"
                        content: viewModel && viewModel.credits ? viewModel.credits : ""
                        Layout.fillWidth: true
                        accent: accentColor
                    }

                    // Copyright Section
                    InfoSection {
                        title: "Copyright"
                        content: viewModel && viewModel.copyright ? viewModel.copyright : ""
                        Layout.fillWidth: true
                        accent: accentColor
                    }

                    // License & System Info
                    InfoSection {
                        title: "License & System Information"
                        content: viewModel && viewModel.license ? viewModel.license : ""
                        Layout.fillWidth: true
                        accent: accentColor
                    }

                    // Qt / Build Info
                    Text {
                        text: viewModel && viewModel.qtVersion ? viewModel.qtVersion : ""
                        font.pixelSize: 12
                        font.family: "Archivo Narrow"
                        color: "#666666"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // footer separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: accentColor
            }

            // BUTTONS ROW
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    text: "View Documentation"
                    Layout.preferredWidth: 180

                    onClicked: {
                        // adjust to your docs path or URL
                        Qt.openUrlExternally("file:///path/to/README.md")
                    }

                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(70/255.0,226/255.0,165/255.0,0.08) : "transparent"
                        border.color: accentColor
                        border.width: 1
                        radius: 0
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        font.family: "Archivo Narrow"
                        color: accentColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true } // spacer

                Button {
                    text: "Close"
                    Layout.preferredWidth: 120

                    onClicked: {
                        if (viewModel) viewModel.visible = false
                    }

                    background: Rectangle {
                        color: parent.hovered ? accentColor : accentColor
                        radius: 0
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        font.bold: true
                        font.family: "Archivo Narrow"
                        color: "#0A0A0A" // black text on accent background
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // InfoSection component (themed, uses the 'accent' property)
    component InfoSection: ColumnLayout {
        property string title: "Section"
        property string content: ""
        property color accent: "#46E2A5"

        spacing: 8

        Text {
            text: title
            font.pixelSize: 18
            font.bold: true
            font.family: "Archivo Narrow"
            color: accent
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333333"
        }

        Text {
            text: content
            font.pixelSize: 14
            font.family: "Archivo Narrow"
            color: "#CCCCCC"
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
