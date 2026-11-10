pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The status bar (04 sections 1 and 5): status only, no Run or Stop. Server state, model, load
// progress and tok/s on the left; one VRAM meter per GPU, the log drawer door and the Settings
// door on the right.
Rectangle {
    id: root

    property bool isLogOpen: false

    signal logToggled
    signal settingsRequested(Item opener)

    implicitHeight: Theme.statusBarHeight
    color: Theme.surface1

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space3
        anchors.rightMargin: Theme.space1
        spacing: Theme.space4

        ServerStateText {}

        Text {
            text: LaunchController.modelName
            visible: text !== ""
            font.family: Theme.uiFamily
            font.pointSize: Theme.captionSize
            color: Theme.text2
            elide: Text.ElideMiddle
            Layout.maximumWidth: Theme.modelPickerWidth
        }

        UsageBar {
            fraction: LaunchController.loadProgress < 0 ? -1 : LaunchController.loadProgress / 100
            visible: LaunchController.state === LaunchController.Loading
            Layout.preferredWidth: Theme.statusMeterWidth
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: qsTr("%1 tok/s").arg(LaunchController.generationTokensPerSecond.toFixed(1))
            visible: LaunchController.state === LaunchController.Serving
            font.family: Theme.monoFamily
            font.pointSize: Theme.captionSize
            color: Theme.text2
        }

        Item {
            Layout.fillWidth: true
        }

        Repeater {
            model: MonitorController.gpus
            delegate: StatusVram {
                required property var modelData
                gpu: modelData
            }
        }

        RowLayout {
            spacing: 0

            GhostButton {
                text: qsTr("Log")
                textColor: root.isLogOpen ? Theme.accent : Theme.text2
                hoverTextColor: root.isLogOpen ? Theme.accent : Theme.text1
                toolTipText: root.isLogOpen ? qsTr("Close the log drawer") : qsTr("Open the log drawer")
                Accessible.role: Accessible.CheckBox
                Accessible.checked: root.isLogOpen
                onClicked: root.logToggled()
            }

            GhostButton {
                id: settingsButton
                text: qsTr("Settings")
                iconName: "settings"
                toolTipText: qsTr("Settings (Ctrl+,)")
                onClicked: root.settingsRequested(settingsButton)
            }
        }
    }
}
