pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// A model's memory split (04 section 4): one bar per tier, GPU0..GPUn, RAM and SSD, in its tier
// color, for the values being edited; PlannerRunner re-plans as they change (05 section 3).
// A plan that fails says why on the line under the bars.
ColumnLayout {
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

    spacing: Theme.space2

    Repeater {
        model: PlannerRunner.tiers
        delegate: RowLayout {
            id: tierRow
            required property var modelData
            spacing: Theme.space3
            Layout.fillWidth: true

            Text {
                text: tierRow.modelData.label
                font.family: Theme.monoFamily
                font.pointSize: Theme.captionSize
                color: Theme.text2
                Layout.preferredWidth: Theme.columnExpertsWidth
            }

            UsageBar {
                fraction: tierRow.modelData.capacityBytes > 0 ? tierRow.modelData.usedBytes / tierRow.modelData.capacityBytes : 0
                fillColor: root.tierColor(tierRow.modelData)
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: tierRow.modelData.label === "SSD" ? qsTr("%1 GB").arg(root.gigabytes(tierRow.modelData.usedBytes))
                                                : qsTr("%1/%2 GB").arg(root.gigabytes(tierRow.modelData.usedBytes))
                                                                  .arg(root.gigabytes(tierRow.modelData.capacityBytes))
                font.family: Theme.monoFamily
                font.pointSize: Theme.captionSize
                color: Theme.text2
                horizontalAlignment: Text.AlignRight
                Layout.preferredWidth: Theme.columnModifiedWidth
            }

            Text {
                text: tierRow.modelData.layerText
                font.family: Theme.monoFamily
                font.pointSize: Theme.captionSize
                color: Theme.text3
                Layout.preferredWidth: implicitWidth
            }
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
