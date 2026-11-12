pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// GPU headroom fields (04 sections 3 and 7.3): one small GB field per GPU in `gpuIndices`, on one
// line. `text` is the stored value ("0:4,1:0.5", GpuHeadroom); an empty field leaves that GPU out
// of it, and its placeholder then shows `fallbackText`'s value for the GPU (or 0).
RowLayout {
    id: root

    property string text
    property string fallbackText
    property var gpuIndices: []

    signal edited(string text)

    function parsed(value: string): var {
        const amounts = {}
        for (const entry of value.split(",")) {
            const parts = entry.split(":")
            if (parts.length === 2 && parts[0].trim() !== "" && parts[1].trim() !== "") {
                amounts[Number(parts[0])] = parts[1].trim()
            }
        }
        return amounts
    }

    function amountFor(value: string, gpuIndex: int): string {
        return root.parsed(value)[gpuIndex] ?? ""
    }

    function withAmount(gpuIndex: int, amount: string): string {
        const amounts = root.parsed(root.text)
        if (amount === "") {
            delete amounts[gpuIndex]
        } else {
            amounts[gpuIndex] = amount
        }
        return Object.keys(amounts).map(Number).sort((a, b) => a - b).map(index => index + ":" + amounts[index]).join(",")
    }

    spacing: Theme.space3

    Repeater {
        model: root.gpuIndices
        delegate: RowLayout {
            id: gpuField
            required property int modelData
            spacing: Theme.space1

            Text {
                text: qsTr("GPU%1").arg(gpuField.modelData)
                font.family: Theme.uiFamily
                font.pointSize: Theme.captionSize
                color: Theme.text2
            }

            NumberField {
                allowDecimal: true
                allowEmpty: true
                fieldHeight: Theme.propertyControlHeight
                placeholderText: root.amountFor(root.fallbackText, gpuField.modelData) || "0"
                text: root.amountFor(root.text, gpuField.modelData)
                Layout.preferredWidth: Theme.columnExpertsWidth
                Layout.fillWidth: false
                onCommitted: value => {
                    const next = root.withAmount(gpuField.modelData, value.trim())
                    if (next !== root.text) {
                        root.edited(next)
                    }
                }
            }
        }
    }
}
