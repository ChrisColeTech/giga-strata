import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Expert usage group of a model's parameters (04 section 4): record usage, and per-expert
// placement with the recorded token count it plans from.
FormGroup {
    id: root

    property real labelWidth
    readonly property var parameters: PlannerRunner.parameters
    readonly property var info: PlannerRunner.modelInfo
    readonly property bool isMoe: Number(info.experts ?? 0) > 0
    readonly property bool hasUsage: Number(info.decodeTokens ?? 0) + Number(info.promptTokens ?? 0) > 0
    readonly property string recordLabel: qsTr("Record usage")
    readonly property string perExpertLabel: qsTr("Per-expert placement")
    readonly property string recordedLabel: qsTr("Recorded tokens")
    readonly property var labels: [recordLabel, perExpertLabel, recordedLabel]

    title: qsTr("Expert usage")
    iconName: "activity"

    FormRow {
        label: root.recordLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Adds up which experts the model uses across sessions; costs about 0.6% speed.")
        disabledReason: root.isMoe ? "" : qsTr("This model has no experts.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.recordLabel
            isOn: root.parameters.record_usage === true
            onSwitched: isOn => PlannerRunner.setParameter("record_usage", isOn)
        }
    }

    FormRow {
        label: root.perExpertLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("Keeps the most used experts of each layer on the GPUs.")
        disabledReason: root.hasUsage ? "" : qsTr("Record expert usage for this model first.")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.perExpertLabel
            isOn: root.parameters.per_expert === true
            onSwitched: isOn => PlannerRunner.setParameter("per_expert", isOn)
        }
    }

    FactRow {
        label: root.recordedLabel
        labelWidth: root.labelWidth
        value: root.hasUsage ? qsTr("%1 decode, %2 prompt").arg(Number(root.info.decodeTokens ?? 0).toLocaleString(Qt.locale(), "f", 0))
                                                         .arg(Number(root.info.promptTokens ?? 0).toLocaleString(Qt.locale(), "f", 0))
                             : ""
        disabledReason: root.hasUsage ? "" : qsTr("No usage has been recorded for this model.")
    }
}
