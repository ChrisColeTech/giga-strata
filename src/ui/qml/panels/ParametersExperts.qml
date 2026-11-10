import QtQuick
import GigaStrata.App

// The EXPERT USAGE section of a model's parameters (04 sections 4 and 7.3): record usage, and
// per-expert placement with the recorded token count it plans from as its status word.
PropertySection {
    id: root

    readonly property var parameters: PlannerRunner.parameters
    readonly property var info: PlannerRunner.modelInfo
    readonly property bool isMoe: Number(info.experts ?? 0) > 0
    readonly property real recordedTokens: Number(info.decodeTokens ?? 0) + Number(info.promptTokens ?? 0)
    readonly property bool hasUsage: recordedTokens > 0

    title: qsTr("Expert usage")

    PropertyRow {
        label: qsTr("Record")
        toolTipText: qsTr("Adds up which experts the model uses across sessions; costs about 0.6% speed.")
        disabledReason: root.isMoe ? "" : qsTr("This model has no experts.")

        Toggle {
            accessibleName: qsTr("Record usage")
            isOn: root.parameters.record_usage === true
            onSwitched: isOn => PlannerRunner.setParameter("record_usage", isOn)
        }
    }

    PropertyRow {
        label: qsTr("Per-expert")
        toolTipText: root.hasUsage
                     ? qsTr("Keeps the most used experts of each layer on the GPUs. Recorded: %1 decode, %2 prompt tokens.")
                       .arg(Number(root.info.decodeTokens ?? 0).toLocaleString(Qt.locale(), "f", 0))
                       .arg(Number(root.info.promptTokens ?? 0).toLocaleString(Qt.locale(), "f", 0))
                     : ""
        status: root.hasUsage ? qsTr("%1 tokens recorded").arg(root.recordedTokens.toLocaleString(Qt.locale(), "f", 0))
                              : qsTr("no usage recorded")
        disabledReason: root.hasUsage ? "" : qsTr("Record expert usage for this model first.")

        Toggle {
            accessibleName: qsTr("Per-expert placement")
            isOn: root.parameters.per_expert === true
            onSwitched: isOn => PlannerRunner.setParameter("per_expert", isOn)
        }
    }
}
