import QtQuick
import QtQuick.Layouts

// One GPU's VRAM meter in the status bar: name, a tier-colored bar, mono `used/total GB`.
RowLayout {
    id: root

    required property var gpu
    readonly property real usedGb: (gpu.totalMib - gpu.freeMib) / 1024
    readonly property real totalGb: gpu.totalMib / 1024

    spacing: Theme.space2

    Text {
        text: qsTr("GPU%1").arg(root.gpu.index)
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.text2
    }

    UsageBar {
        fraction: root.totalGb > 0 ? root.usedGb / root.totalGb : 0
        fillColor: Theme.tierGpu[root.gpu.index % Theme.tierGpu.length]
        Layout.preferredWidth: Theme.statusMeterWidth
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        text: qsTr("%1/%2 GB").arg(root.usedGb.toFixed(1)).arg(root.totalGb.toFixed(1))
        font.family: Theme.monoFamily
        font.pointSize: Theme.captionSize
        color: Theme.text2
    }

    HoverHandler {
        id: hover
    }

    ThemedToolTip {
        visible: hover.hovered
        text: root.gpu.name
    }
}
