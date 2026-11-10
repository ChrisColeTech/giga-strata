pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The GPUs row of a model's parameters (04 section 7.3): one compact checkbox per detected GPU on
// one line; a click includes or leaves out that GPU. `gpus` is the saved list ("0,1": nvidia-smi
// indices in the order they become CUDA0, CUDA1, ...); the tooltip says which CUDA device it runs
// as and how much of it is free.
RowLayout {
    id: root

    property string gpus
    readonly property var order: gpus.split(",").map(each => each.trim()).filter(each => each !== "").map(Number)

    signal edited(string gpus)

    function toggled(gpuIndex: int): string {
        const isIncluded = root.order.indexOf(gpuIndex) >= 0
        const next = isIncluded ? root.order.filter(each => each !== gpuIndex) : root.order.concat([gpuIndex])
        return next.join(",")
    }

    spacing: 0

    Repeater {
        model: MonitorController.gpus
        delegate: CheckRow {
            id: gpuBox
            required property var modelData
            required property int index
            readonly property int cudaOrder: root.order.indexOf(modelData.index)
            readonly property string freeText: qsTr("%1 of %2 GB free").arg((modelData.freeMib / 1024).toFixed(1))
                                                                       .arg((modelData.totalMib / 1024).toFixed(1))

            // "GPU0 5090": the model number alone keeps two GPUs on one line of a 420px panel.
            text: qsTr("GPU%1 %2").arg(modelData.index).arg(String(modelData.shortName).split(" ").pop())
            isTicked: cudaOrder >= 0
            // The first box lines up with the label column's controls, not with its hover inset.
            Layout.leftMargin: index === 0 ? -Theme.space3 : 0
            onTickToggled: root.edited(root.toggled(modelData.index))

            ThemedToolTip {
                visible: gpuBox.hovered
                text: gpuBox.isTicked ? qsTr("%1 runs as CUDA%2; %3.").arg(gpuBox.modelData.name).arg(gpuBox.cudaOrder).arg(gpuBox.freeText)
                                      : qsTr("%1 is not used; %2.").arg(gpuBox.modelData.name).arg(gpuBox.freeText)
            }
        }
    }
}
