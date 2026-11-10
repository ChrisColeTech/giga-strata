pragma ComponentBehavior: Bound

import QtQuick
import GigaStrata.App

// The GPUs row of a model's parameters (S3): one chip per detected GPU; a click includes or
// leaves out that GPU. `gpus` is the saved list ("0,1": nvidia-smi indices in the order they
// become CUDA0, CUDA1, ...); an included GPU's dot takes its CUDA order's tier color.
Flow {
    id: root

    property string gpus
    readonly property var order: gpus.split(",").map(each => each.trim()).filter(each => each !== "").map(Number)

    signal edited(string gpus)

    function toggled(gpuIndex: int): string {
        const isIncluded = root.order.indexOf(gpuIndex) >= 0
        const next = isIncluded ? root.order.filter(each => each !== gpuIndex) : root.order.concat([gpuIndex])
        return next.join(",")
    }

    spacing: Theme.space2

    Repeater {
        model: MonitorController.gpus
        delegate: Chip {
            required property var modelData
            readonly property int cudaOrder: root.order.indexOf(modelData.index)

            text: qsTr("GPU%1 · %2").arg(modelData.index).arg(modelData.shortName)
            valueText: qsTr("%1/%2 GB free").arg((modelData.freeMib / 1024).toFixed(0)).arg((modelData.totalMib / 1024).toFixed(0))
            isOn: cudaOrder >= 0
            dotColor: isOn ? Theme.tierGpu[cudaOrder % Theme.tierGpu.length] : Theme.text3
            toolTipText: isOn ? qsTr("%1 runs as CUDA%2. Click to leave it out.").arg(modelData.name).arg(cudaOrder)
                              : qsTr("%1 is not used. Click to include it.").arg(modelData.name)
            onSwitched: root.edited(root.toggled(modelData.index))
        }
    }
}
