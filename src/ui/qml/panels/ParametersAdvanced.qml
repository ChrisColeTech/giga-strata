import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Advanced group of a model's parameters (04 section 4): margin, micro-batch, SSD streaming,
// flash attention, extra arguments, and Restore defaults at its end. Folded at first (P3).
FormGroup {
    id: root

    property real labelWidth
    readonly property var parameters: PlannerRunner.parameters
    readonly property string marginLabel: qsTr("Safety margin (MiB)")
    readonly property string ubatchLabel: qsTr("Micro-batch")
    readonly property string mmapLabel: qsTr("SSD streaming")
    readonly property string flashLabel: qsTr("Flash attention")
    readonly property string extraLabel: qsTr("Extra arguments")
    readonly property var labels: [marginLabel, ubatchLabel, mmapLabel, flashLabel, extraLabel]

    title: qsTr("Advanced")
    iconName: "wrench"
    isLast: true
    isOpen: false

    FormRow {
        label: root.marginLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("VRAM kept free per GPU on top of the estimated compute buffers. Raise it if the server crashes.")
        Layout.fillWidth: true

        NumberField {
            placeholderText: "1536"
            text: String(root.parameters.margin ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("margin", value.trim())
        }
    }

    FormRow {
        label: root.ubatchLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("auto = 2048 if all experts fit in VRAM, else 512 when that fits more.")
        Layout.fillWidth: true

        NumberField {
            placeholderText: "auto"
            allowAuto: true
            minimum: 1
            text: String(root.parameters.ubatch ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("ubatch", value.trim() === "" ? "auto" : value.trim())
        }
    }

    FormRow {
        label: root.mmapLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("When the RAM tier does not fit in free RAM, stream it from disk instead of loading it up front.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.mmapLabel
            isOn: root.parameters.mmap === true
            onSwitched: isOn => PlannerRunner.setParameter("mmap", isOn)
        }
    }

    FormRow {
        label: root.flashLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Passes -fa on, which lowers the attention VRAM overhead.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.flashLabel
            isOn: root.parameters.fa === true
            onSwitched: isOn => PlannerRunner.setParameter("fa", isOn)
        }
    }

    FormRow {
        label: root.extraLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Appended to the llama-server command line, split like a Windows command line.")
        Layout.fillWidth: true

        TextField {
            placeholderText: qsTr("extra flags…")
            text: String(root.parameters.extra ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("extra", value)
        }
    }

    GhostButton {
        text: qsTr("Restore defaults")
        iconName: "rotate-ccw"
        toolTipText: qsTr("Puts every parameter of this model back to the automatic defaults; Save keeps them.")
        Layout.alignment: Qt.AlignRight
        onClicked: PlannerRunner.resetToDefaults()
    }
}
