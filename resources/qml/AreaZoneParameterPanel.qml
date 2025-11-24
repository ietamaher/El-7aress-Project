import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    // Public viewModel (bind your zoneDefinitionViewModel here)
    property var viewModel: null

    // Safe accent color default (hex). If your viewModel supplies a different color string, it will override.
    property color accentColor: viewModel && viewModel.accentColor ? viewModel.accentColor : "#46E2A5"

    // Derived safe viewModel properties to avoid undefined accesses
    property int vmActiveField: viewModel && viewModel.activeField !== undefined ? viewModel.activeField : -1
    property bool vmEnabled: viewModel ? !!viewModel.isEnabled : false
    property bool vmOverridable: viewModel ? !!viewModel.isOverridable : false

    // Pre-computed highlight/back colors (keeps visuals consistent even if accentColor is a string)
    property color highlightBg: Qt.rgba(70/255.0, 226/255.0, 165/255.0, 0.1)
    property color dangerColor: "#C81428"
    property color dangerBg: Qt.rgba(200/255.0, 20/255.0, 40/255.0, 0.1)

    color: "#0A0A0A"
    radius: 0
    border.color: "transparent"
    border.width: 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // Title section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "transparent"

            Text {
                anchors.centerIn: parent
                text: "Area Zone Parameters"
                font.pixelSize: 18
                font.weight: Font.Normal
                font.family: "Segoe UI"
                color: "#FFFFFF"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#151515"
        }

        // Parameter fields container with fixed height
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 104  // 2 rows * 52
            color: "transparent"
            clip: true  // Prevent overflow

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Enabled field
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    clip: true

                    // Row hover background
                    Rectangle {
                        anchors.fill: parent
                        color: mouseArea1.containsMouse && vmActiveField !== 0 ? "#15FFFFFF" : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }                    

                    // Accent left strip (anchored to row bounds)
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 4
                        color: accentColor
                        visible: vmActiveField === 0
                        opacity: vmActiveField === 0 ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 28
                        anchors.rightMargin: 20
                        spacing: 12

                        Text {
                            text: "Enabled:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 0 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 0 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        // Checkbox visual
                        Rectangle {
                            width: 20
                            height: 20
                            radius: 3
                            border.color: vmEnabled ? accentColor : "#404040"
                            border.width: 2
                            color: vmEnabled ? accentColor : "transparent"

                            Behavior on border.color { ColorAnimation { duration: 200 } }
                            Behavior on color { ColorAnimation { duration: 200 } }

                            Text {
                                text: "✓"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#0A0A0A"
                                anchors.centerIn: parent
                                visible: vmEnabled
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        width: parent.width - 28
                        height: 1
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        color: "#151515"
                    }

                    MouseArea {
                        id: mouseArea1
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (viewModel) viewModel.isEnabled = !viewModel.isEnabled
                        }
                    }
                }

                // Overridable row
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        color: mouseArea2.containsMouse && vmActiveField !== 1 ? "#15FFFFFF" : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 4
                        color: accentColor
                        visible: vmActiveField === 1
                        opacity: vmActiveField === 1 ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 28
                        anchors.rightMargin: 20
                        spacing: 12

                        Text {
                            text: "Overridable:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 1 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 1 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        Rectangle {
                            width: 20
                            height: 20
                            radius: 3
                            border.color: vmOverridable ? accentColor : "#404040"
                            border.width: 2
                            color: vmOverridable ? accentColor : "transparent"

                            Behavior on border.color { ColorAnimation { duration: 200 } }
                            Behavior on color { ColorAnimation { duration: 200 } }

                            Text {
                                text: "✓"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#0A0A0A"
                                anchors.centerIn: parent
                                visible: vmOverridable
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        width: parent.width - 28
                        height: 1
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        color: "#151515"
                    }

                    MouseArea {
                        id: mouseArea2
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (viewModel) viewModel.isOverridable = !viewModel.isOverridable
                        }
                    }
                }
            }
        }

      // Spacer
        Item { Layout.fillHeight: true }

        // Bottom action buttons divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#151515"
        }

        // Buttons row (fixed height) with proper clipping
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.maximumHeight: 52
            spacing: 0

            // Validate button
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: vmActiveField === 2 ? highlightBg : "transparent"
                clip: true

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 4
                    color: accentColor
                    visible: vmActiveField === 2
                }

                Text {
                    anchors.centerIn: parent
                    text: "Validate"
                    font.pixelSize: 16
                    font.weight: vmActiveField === 2 ? Font.DemiBold : Font.Normal
                    font.family: "Segoe UI"
                    color: vmActiveField === 2 ? "#FFFFFF" : "#808080"
                }
            }

            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: "#151515"
            }

            // Cancel button
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: vmActiveField === 3 ? dangerBg : "transparent"
                clip: true

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 4
                    color: dangerColor
                    visible: vmActiveField === 3
                }

                Text {
                    anchors.centerIn: parent
                    text: "Cancel"
                    font.pixelSize: 16
                    font.weight: vmActiveField === 3 ? Font.DemiBold : Font.Normal
                    font.family: "Segoe UI"
                    color: vmActiveField === 3 ? dangerColor : "#808080"
                }
            }
        }

        // Instructions footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "#0D0D0D"

            Text {
                anchors.centerIn: parent
                text: "UP/DOWN to navigate • MENU/VAL to toggle/confirm"
                font.pixelSize: 10
                font.family: "Segoe UI"
                color: "#606060"
            }
        }
    }
}
