import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Main group of a model's parameters (04 section 4): GPUs, context, MTP, vision and its
// projector. Values are PlannerRunner.parameters (05 section 3); every edit re-plans live.
FormGroup {
    id: root

    property real labelWidth
    readonly property var parameters: PlannerRunner.parameters
    readonly property var contextValues: ["8192", "32768", "65536", "131072", "200000"]
    readonly property int contextIndex: contextValues.indexOf(String(parameters.ctx ?? ""))
    property bool isCustomContext: false
    readonly property string gpusLabel: qsTr("GPUs")
    readonly property string contextLabel: qsTr("Context")
    readonly property string customLabel: qsTr("Context tokens")
    readonly property string mtpLabel: qsTr("MTP speculation")
    readonly property string visionLabel: qsTr("Image input")
    readonly property string projectorLabel: qsTr("Vision projector")
    readonly property var labels: [gpusLabel, contextLabel, customLabel, mtpLabel, visionLabel, projectorLabel]

    title: qsTr("Main")
    iconName: "sliders-horizontal"

    FormRow {
        label: root.gpusLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Included GPUs become CUDA0, CUDA1, ... in the order you include them.")
        disabledReason: MonitorController.gpus.length > 0 ? "" : MonitorController.gpuError
        Layout.fillWidth: true

        GpuChips {
            gpus: String(root.parameters.gpus ?? "")
            // Preferred 0: the row hands the chips its width and they wrap inside it.
            Layout.preferredWidth: 0
            Layout.fillWidth: true
            onEdited: gpus => PlannerRunner.setParameter("gpus", gpus)
        }
    }

    FormRow {
        label: root.contextLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Tokens of context; its KV cache is reserved on the GPUs first.")
        Layout.fillWidth: true

        SegmentedControl {
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

    FormRow {
        label: root.customLabel
        labelWidth: root.labelWidth
        visible: root.isCustomContext || root.contextIndex < 0
        Layout.fillWidth: true

        NumberField {
            placeholderText: "32768"
            minimum: 1
            text: String(root.parameters.ctx ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("ctx", value.trim())
        }
    }

    FormRow {
        label: root.mtpLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Speculates with the model's MTP draft.")
        disabledReason: String(PlannerRunner.modelInfo.draftPath ?? "") !== "" ? "" : qsTr("No MTP draft was found for this model.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.mtpLabel
            isOn: root.parameters.mtp === true
            onSwitched: isOn => PlannerRunner.setParameter("mtp", isOn)
        }
    }

    FormRow {
        label: root.visionLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Loads a vision projector on the last GPU.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.visionLabel
            isOn: root.parameters.vision === true
            onSwitched: isOn => PlannerRunner.setParameter("vision", isOn)
        }
    }

    FormRow {
        label: root.projectorLabel
        labelWidth: root.labelWidth
        visible: root.parameters.vision === true
        Layout.fillWidth: true

        PathField {
            placeholderText: String(PlannerRunner.modelInfo.projectorPath ?? "") || qsTr("mmproj GGUF")
            dialogTitle: qsTr("Select a vision projector (mmproj) GGUF")
            text: String(root.parameters.mmproj ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("mmproj", value.trim())
            onPicked: fileUrl => PlannerRunner.setParameter("mmproj", SettingsStore.nativePath(fileUrl))
        }
    }
}
