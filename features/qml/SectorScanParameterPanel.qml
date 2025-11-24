import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    // Public viewModel binding
    property var viewModel: null

    // Safe accent color with fallback
    property color accentColor: viewModel && viewModel.accentColor ? viewModel.accentColor : "#46E2A5"

    // Safe viewModel property access with defaults
    property int vmActiveField: viewModel && viewModel.activeField !== undefined ? viewModel.activeField : -1
    property bool vmEnabled: viewModel ? !!viewModel.isEnabled : false
    property int vmScanSpeed: viewModel && viewModel.scanSpeed !== undefined ? viewModel.scanSpeed : 5
    property bool vmIsEditingValue: viewModel ? !!viewModel.isEditingValue : false

    // Pre-computed colors for consistency
    property color highlightBg: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1)
    property color dangerColor: "#C81428"
    property color dangerBg: Qt.rgba(200/255.0, 20/255.0, 40/255.0, 0.1)
    property color editIndicatorColor: "#FF9800"
    property color editIndicatorBg: Qt.rgba(255/255.0, 152/255.0, 0/255.0, 0.2)

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
                text: "Sector Scan Parameters"
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

                // Scan Speed field
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
                            text: "Scan Speed:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 1 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 1 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            text: vmScanSpeed + " deg/s"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.family: "Segoe UI"
                            color: vmActiveField === 1 ? accentColor : "#808080"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        Rectangle {
                            width: 50
                            height: 20
                            radius: 3
                            color: editIndicatorBg
                            border.color: editIndicatorColor
                            border.width: 1
                            visible: vmIsEditingValue && vmActiveField === 1

                            Text {
                                anchors.centerIn: parent
                                text: "EDIT"
                                font.pixelSize: 10
                                font.family: "Segoe UI"
                                font.weight: Font.DemiBold
                                color: editIndicatorColor
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
                            if (viewModel && viewModel.selectField) viewModel.selectField(1)
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
                text: vmIsEditingValue ?
                      "UP/DOWN to adjust value • MENU/VAL to confirm" :
                      "UP/DOWN to navigate • MENU/VAL to edit/confirm"
                font.pixelSize: 10
                font.family: "Segoe UI"
                color: "#606060"
            }
        }
    }
}
