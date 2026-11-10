pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// A model's memory split (04 sections 4 and 7.3), first in the grid: one bar row per tier,
// GPU0..GPUn, RAM and SSD, in its tier color, for the values being edited; PlannerRunner re-plans
// live as they change (05 section 3). A tier's layers are in its label's tooltip. A plan that
// fails says why on the line under the bars.
PropertySection {
    id: root

    function tierColor(tier: var): color {
        if (tier.label === "RAM") {
            return Theme.tierRam
        }
        if (tier.label === "SSD") {
            return Theme.tierSsd
        }
        return Theme.tierGpu[tier.tierIndex % Theme.tierGpu.length]
    }

    function gigabytes(bytes: real): string {
        return (bytes / (1024 * 1024 * 1024)).toFixed(1)
    }

    title: qsTr("Memory")

    Repeater {
        model: PlannerRunner.tiers
        delegate: MeterRow {
            required property var modelData

            label: modelData.label
            toolTipText: modelData.layerText
            fraction: modelData.capacityBytes > 0 ? modelData.usedBytes / modelData.capacityBytes : 0
            fillColor: root.tierColor(modelData)
            valueText: modelData.label === "SSD" ? qsTr("%1 GB").arg(root.gigabytes(modelData.usedBytes))
                                                 : qsTr("%1 / %2 GB").arg(root.gigabytes(modelData.usedBytes))
                                                                     .arg(root.gigabytes(modelData.capacityBytes))
        }
    }

    Text {
        text: PlannerRunner.errorText
        visible: text !== ""
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.danger
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
}
