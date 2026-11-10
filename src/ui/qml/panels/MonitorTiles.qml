pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving monitor tiles (04 section 5, row 3): Speed (generation tok/s, prompt tok/s under
// it), Uptime, one tile per GPU (VRAM bar, used/total GB), RAM in use. Bound to the same
// controller properties as the status bar; the row shrinks when the Parameters panel opens.
RowLayout {
    id: root

    readonly property real bytesPerGb: 1024 * 1024 * 1024

    function uptime(seconds: int): string {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor(seconds % 3600 / 60)
        const rest = seconds % 60
        return [hours, minutes, rest].map(part => String(part).padStart(2, "0")).join(":")
    }

    spacing: Theme.space3

    MonitorTile {
        title: qsTr("Speed")
        value: qsTr("%1 tok/s").arg(LaunchController.generationTokensPerSecond.toFixed(1))
        detail: qsTr("prompt %1 tok/s").arg(LaunchController.promptTokensPerSecond.toFixed(1))
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    MonitorTile {
        title: qsTr("Uptime")
        value: root.uptime(LaunchController.uptimeSeconds)
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Repeater {
        model: MonitorController.gpus
        delegate: MonitorTile {
            required property var modelData
            readonly property real usedGb: (modelData.totalMib - modelData.freeMib) / 1024
            readonly property real totalGb: modelData.totalMib / 1024

            title: qsTr("GPU%1 · %2").arg(modelData.index).arg(modelData.shortName)
            value: qsTr("%1/%2 GB").arg(usedGb.toFixed(1)).arg(totalGb.toFixed(1))
            fraction: totalGb > 0 ? usedGb / totalGb : 0
            barColor: Theme.tierGpu[modelData.index % Theme.tierGpu.length]
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    MonitorTile {
        readonly property real usedBytes: MonitorController.totalRamBytes - MonitorController.freeRamBytes

        title: qsTr("RAM")
        value: MonitorController.hasRam ? qsTr("%1 GB").arg((usedBytes / root.bytesPerGb).toFixed(1)) : "—"
        fraction: MonitorController.hasRam && MonitorController.totalRamBytes > 0
                  ? usedBytes / MonitorController.totalRamBytes : 0
        barColor: Theme.tierRam
        Layout.fillWidth: true
        Layout.fillHeight: true
    }
}
