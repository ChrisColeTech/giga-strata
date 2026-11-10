pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving readout block (04 section 7.2): Speed (with prompt), Uptime and the URL with Copy
// on the left; one thin bar row per GPU and RAM on the right. Bound to the same controller
// properties as the status bar. Speed and Uptime describe a running server, so once it stops
// they read a dash instead of the last values.
RowLayout {
    id: root

    readonly property real bytesPerGb: 1024 * 1024 * 1024
    readonly property int serverState: LaunchController.state
    readonly property bool isServing: serverState === LaunchController.Serving
    readonly property bool isRunning: isServing || serverState === LaunchController.Starting
                                      || serverState === LaunchController.Loading

    function uptime(seconds: int): string {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor(seconds % 3600 / 60)
        const rest = seconds % 60
        return [hours, minutes, rest].map(part => String(part).padStart(2, "0")).join(":")
    }

    spacing: Theme.space6

    ColumnLayout {
        spacing: 0
        Layout.preferredWidth: 0
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        PropertyRow {
            label: qsTr("Speed")
            labelWidth: Theme.readoutLabelWidth
            toolTipText: qsTr("Generation tokens per second, then prompt processing.")

            Text {
                text: root.isServing ? qsTr("%1 tok/s").arg(LaunchController.generationTokensPerSecond.toFixed(1)) : "—"
                font.family: Theme.monoFamily
                font.pointSize: Theme.bodySize
                color: root.isServing ? Theme.text1 : Theme.text3
            }

            Text {
                text: qsTr("· prompt %1").arg(LaunchController.promptTokensPerSecond.toFixed(1))
                visible: root.isServing
                font.family: Theme.monoFamily
                font.pointSize: Theme.bodySize
                color: Theme.text2
            }
        }

        PropertyRow {
            label: qsTr("Uptime")
            labelWidth: Theme.readoutLabelWidth

            Text {
                text: root.isRunning ? root.uptime(LaunchController.uptimeSeconds) : "—"
                font.family: Theme.monoFamily
                font.pointSize: Theme.bodySize
                color: root.isRunning ? Theme.text1 : Theme.text3
            }
        }

        PropertyRow {
            label: qsTr("URL")
            labelWidth: Theme.readoutLabelWidth
            isControlWide: true

            Text {
                text: LaunchController.url === "" ? "—" : LaunchController.url
                font.family: Theme.monoFamily
                font.pointSize: Theme.bodySize
                color: LaunchController.url === "" ? Theme.text3 : Theme.text1
                elide: Text.ElideRight
                Layout.maximumWidth: implicitWidth
                Layout.fillWidth: true
            }

            IconButton {
                iconName: "copy"
                iconSize: Theme.iconSize - Theme.space1
                implicitWidth: Theme.propertyControlHeight
                implicitHeight: Theme.propertyControlHeight
                visible: LaunchController.url !== ""
                toolTipText: qsTr("Copy the URL")
                onClicked: LaunchController.copyUrl()
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    ColumnLayout {
        spacing: 0
        Layout.preferredWidth: 0
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        Repeater {
            model: MonitorController.gpus
            delegate: MeterRow {
                required property var modelData
                readonly property real usedGb: (modelData.totalMib - modelData.freeMib) / 1024
                readonly property real totalGb: modelData.totalMib / 1024

                label: qsTr("GPU%1").arg(modelData.index)
                toolTipText: modelData.name
                fraction: totalGb > 0 ? usedGb / totalGb : 0
                fillColor: Theme.tierGpu[modelData.index % Theme.tierGpu.length]
                valueText: qsTr("%1 / %2 GB").arg(usedGb.toFixed(1)).arg(totalGb.toFixed(1))
            }
        }

        MeterRow {
            readonly property real usedBytes: MonitorController.totalRamBytes - MonitorController.freeRamBytes

            label: qsTr("RAM")
            fraction: MonitorController.hasRam && MonitorController.totalRamBytes > 0
                      ? usedBytes / MonitorController.totalRamBytes : 0
            fillColor: Theme.tierRam
            valueText: MonitorController.hasRam ? qsTr("%1 GB used").arg((usedBytes / root.bytesPerGb).toFixed(1)) : "—"
        }
    }
}
