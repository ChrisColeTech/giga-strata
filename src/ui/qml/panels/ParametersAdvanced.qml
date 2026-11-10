import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The ADVANCED section of a model's parameters (04 sections 4 and 7.3), folded at first: margin,
// micro-batch, SSD streaming and flash attention on one row, extra arguments, and Restore
// defaults at its end.
PropertySection {
    id: root

    readonly property var parameters: PlannerRunner.parameters

    title: qsTr("Advanced")
    isCollapsible: true
    isOpen: false

    PropertyRow {
        label: qsTr("Margin (MiB)")
        toolTipText: qsTr("VRAM kept free per GPU on top of the estimated compute buffers. Raise it if the server crashes.")

        NumberField {
            placeholderText: "1536"
            fieldHeight: Theme.propertyControlHeight
            text: String(root.parameters.margin ?? "")
            Layout.preferredWidth: Theme.columnSizeWidth
            Layout.fillWidth: false
            onCommitted: value => PlannerRunner.setParameter("margin", value.trim())
        }
    }

    PropertyRow {
        label: qsTr("Micro-batch")
        toolTipText: qsTr("auto = 2048 if all experts fit in VRAM, else 512 when that fits more.")

        NumberField {
            placeholderText: "auto"
            allowAuto: true
            minimum: 1
            fieldHeight: Theme.propertyControlHeight
            text: String(root.parameters.ubatch ?? "")
            Layout.preferredWidth: Theme.columnSizeWidth
            Layout.fillWidth: false
            onCommitted: value => PlannerRunner.setParameter("ubatch", value.trim() === "" ? "auto" : value.trim())
        }
    }

    PropertyRow {
        label: qsTr("SSD stream")
        toolTipText: qsTr("SSD stream: when the RAM tier does not fit in free RAM, stream it from disk instead of loading it up front. Flash attn passes -fa on, which lowers the attention VRAM overhead.")

        Toggle {
            accessibleName: qsTr("SSD streaming")
            isOn: root.parameters.mmap === true
            onSwitched: isOn => PlannerRunner.setParameter("mmap", isOn)
        }

        Text {
            text: qsTr("Flash attn")
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            color: Theme.text2
            Layout.leftMargin: Theme.space4
        }

        Toggle {
            accessibleName: qsTr("Flash attention")
            isOn: root.parameters.fa === true
            onSwitched: isOn => PlannerRunner.setParameter("fa", isOn)
        }
    }

    PropertyRow {
        label: qsTr("Extra args")
        toolTipText: qsTr("Appended to the llama-server command line, split like a Windows command line.")
        isControlWide: true

        TextField {
            placeholderText: qsTr("extra flags…")
            fieldHeight: Theme.propertyControlHeight
            text: String(root.parameters.extra ?? "")
            Layout.fillWidth: true
            onCommitted: value => PlannerRunner.setParameter("extra", value)
        }
    }

    GhostButton {
        text: qsTr("Restore defaults")
        iconName: "rotate-ccw"
        implicitHeight: Theme.propertyRowHeight
        toolTipText: qsTr("Puts every parameter of this model back to the automatic defaults; Save keeps them.")
        Layout.alignment: Qt.AlignRight
        onClicked: PlannerRunner.resetToDefaults()
    }
}
