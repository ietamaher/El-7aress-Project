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
    property string vmLocationPage: viewModel && viewModel.locationPage !== undefined ? String(viewModel.locationPage) : "1"
    property string vmTrpInPage: viewModel && viewModel.trpInPage !== undefined ? String(viewModel.trpInPage) : "1"
    property real vmHaltTime: viewModel && viewModel.haltTime !== undefined ? viewModel.haltTime : 1.0
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
                text: "TRP Parameters"
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
            Layout.preferredHeight: 156  // 3 rows * 52
            color: "transparent"
            clip: true  // Prevent overflow

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Location Page field
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        color: mouseArea1.containsMouse && vmActiveField !== 0 ? "#15FFFFFF" : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

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
                            text: "Location Page:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 0 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 0 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            text: vmLocationPage
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.family: "Segoe UI"
                            color: vmActiveField === 0 ? accentColor : "#808080"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        Rectangle {
                            width: 50
                            height: 20
                            radius: 3
                            color: editIndicatorBg
                            border.color: editIndicatorColor
                            border.width: 1
                            visible: vmIsEditingValue && vmActiveField === 0

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
                        id: mouseArea1
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (viewModel && viewModel.selectField) viewModel.selectField(0)
                        }
                    }
                }

                // TRP Index field
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
                            text: "TRP Index:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 1 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 1 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            text: vmTrpInPage
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

                // Halt Time field
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        color: mouseArea3.containsMouse && vmActiveField !== 2 ? "#15FFFFFF" : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 4
                        color: accentColor
                        visible: vmActiveField === 2
                        opacity: vmActiveField === 2 ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 28
                        anchors.rightMargin: 20
                        spacing: 12

                        Text {
                            text: "Halt Time:"
                            font.pixelSize: 15
                            font.weight: vmActiveField === 2 ? Font.DemiBold : Font.Normal
                            font.family: "Segoe UI"
                            color: vmActiveField === 2 ? "#FFFFFF" : "#CCCCCC"
                            Behavior on color { ColorAnimation { duration: 150 } }
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            text: vmHaltTime.toFixed(1) + " sec"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.family: "Segoe UI"
                            color: vmActiveField === 2 ? accentColor : "#808080"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        Rectangle {
                            width: 50
                            height: 20
                            radius: 3
                            color: editIndicatorBg
                            border.color: editIndicatorColor
                            border.width: 1
                            visible: vmIsEditingValue && vmActiveField === 2

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
                        id: mouseArea3
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (viewModel && viewModel.selectField) viewModel.selectField(2)
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
                color: vmActiveField === 3 ? highlightBg : "transparent"
                clip: true

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 4
                    color: accentColor
                    visible: vmActiveField === 3
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: if (viewModel && viewModel.onValidate) viewModel.onValidate()
                }

                Text {
                    anchors.centerIn: parent
                    text: "Validate"
                    font.pixelSize: 16
                    font.weight: vmActiveField === 3 ? Font.DemiBold : Font.Normal
                    font.family: "Segoe UI"
                    color: vmActiveField === 3 ? "#FFFFFF" : "#808080"
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
                color: vmActiveField === 4 ? dangerBg : "transparent"
                clip: true

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 4
                    color: dangerColor
                    visible: vmActiveField === 4
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: if (viewModel && viewModel.onCancel) viewModel.onCancel()
                }

                Text {
                    anchors.centerIn: parent
                    text: "Cancel"
                    font.pixelSize: 16
                    font.weight: vmActiveField === 4 ? Font.DemiBold : Font.Normal
                    font.family: "Segoe UI"
                    color: vmActiveField === 4 ? dangerColor : "#808080"
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
