import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The MAIN section of a model's parameters (04 sections 4 and 7.3): GPUs, their headroom, context,
// MTP, image input and its projector. Values are PlannerRunner.parameters (05 section 3); every edit
// re-plans live.
PropertySection {
    id: root

    readonly property var parameters: PlannerRunner.parameters
    readonly property var info: PlannerRunner.modelInfo
    readonly property var contextValues: ["8192", "32768", "65536", "131072", "200000"]
    readonly property int contextIndex: contextValues.indexOf(String(parameters.ctx ?? ""))
    readonly property bool hasDraft: String(info.draftPath ?? "") !== ""
    readonly property bool hasProjector: String(info.projectorPath ?? "") !== ""
    property bool isCustomContext: false

    title: qsTr("Main")

    PropertyRow {
        label: qsTr("GPUs")
        toolTipText: qsTr("Included GPUs become CUDA0, CUDA1, ... in the order you include them.")
        disabledReason: MonitorController.gpus.length > 0 ? "" : MonitorController.gpuError

        GpuChecks {
            gpus: String(root.parameters.gpus ?? "")
            onEdited: gpus => PlannerRunner.setParameter("gpus", gpus)
        }
    }

    PropertyRow {
        label: qsTr("Headroom (GB)")
        toolTipText: qsTr("VRAM kept free on each GPU for other programs. Empty uses the GPU headroom in Settings > Server.")
        visible: String(root.parameters.gpus ?? "") !== ""

        GpuHeadroomFields {
            text: String(root.parameters.headroom ?? "")
            fallbackText: SettingsStore.gpuHeadroom
            gpuIndices: String(root.parameters.gpus ?? "").split(",").map(each => each.trim()).filter(each => each !== "").map(Number)
            onEdited: text => PlannerRunner.setParameter("headroom", text)
        }
    }

    PropertyRow {
        label: qsTr("Context")
        toolTipText: qsTr("Tokens of context; its KV cache is reserved on the GPUs first.")

        SegmentedControl {
            isCompact: true
            model: ["8K", "32K", "64K", "128K", "200K", qsTr("Custom")]
            currentIndex: root.isCustomContext || root.contextIndex < 0 ? root.contextValues.length : root.contextIndex
            onActivated: index => {
                root.isCustomContext = index === root.contextValues.length
                if (!root.isCustomContext) {
                    PlannerRunner.setParameter("ctx", root.contextValues[index])
                }
            }
        }
    }

    PropertyRow {
        label: qsTr("Context tokens")
        visible: root.isCustomContext || root.contextIndex < 0

        NumberField {
            placeholderText: "32768"
            minimum: 1
            fieldHeight: Theme.propertyControlHeight
            text: String(root.parameters.ctx ?? "")
            Layout.preferredWidth: Theme.columnSizeWidth
            Layout.fillWidth: false
            onCommitted: value => PlannerRunner.setParameter("ctx", value.trim())
        }
    }

    PropertyRow {
        label: qsTr("MTP")
        toolTipText: qsTr("Speculates with the model's MTP draft.")
        status: root.hasDraft ? qsTr("draft found") : qsTr("no draft")
        disabledReason: root.hasDraft ? "" : qsTr("No MTP draft was found for this model.")

        Toggle {
            accessibleName: qsTr("MTP speculation")
            isOn: root.parameters.mtp === true
            onSwitched: isOn => PlannerRunner.setParameter("mtp", isOn)
        }
    }

    PropertyRow {
        label: qsTr("Image input")
        toolTipText: qsTr("Loads a vision projector on the last GPU.")
        status: root.hasProjector ? qsTr("projector found") : ""

        Toggle {
            accessibleName: qsTr("Image input")
            isOn: root.parameters.vision === true
            onSwitched: isOn => PlannerRunner.setParameter("vision", isOn)
        }
    }

    PropertyRow {
        label: qsTr("Projector")
        toolTipText: qsTr("The vision projector (mmproj) GGUF; empty uses the one found beside the model.")
        visible: root.parameters.vision === true
        isControlWide: true

        PathField {
            placeholderText: String(root.info.projectorPath ?? "") || qsTr("mmproj GGUF")
            dialogTitle: qsTr("Select a vision projector (mmproj) GGUF")
            fieldHeight: Theme.propertyControlHeight
            text: String(root.parameters.mmproj ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("mmproj", value.trim())
            onPicked: fileUrl => PlannerRunner.setParameter("mmproj", SettingsStore.nativePath(fileUrl))
        }
    }
}
