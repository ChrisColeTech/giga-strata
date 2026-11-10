import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// A model's parameters as one family form (04 sections 4 and 5): Main, Expert usage and
// Advanced, one label column for all three, and the one commit at the end of the last group
// (Save in the Model config window, Save & Reload in the Parameters panel).
ColumnLayout {
    id: root

    property string commitText
    property string commitToolTip
    readonly property real labelWidth: labelColumn.width

    signal commitRequested

    spacing: 0

    LabelColumn {
        id: labelColumn
        labels: main.labels.concat(experts.labels, advanced.labels)
    }

    ParametersMain {
        id: main
        labelWidth: root.labelWidth
        Layout.fillWidth: true
    }

    ParametersExperts {
        id: experts
        labelWidth: root.labelWidth
        Layout.fillWidth: true
    }

    ParametersAdvanced {
        id: advanced
        labelWidth: root.labelWidth
        Layout.fillWidth: true
    }

    FormActions {
        PrimaryButton {
            text: root.commitText
            toolTipText: root.commitToolTip
            enabled: PlannerRunner.modelPath !== ""
            onClicked: root.commitRequested()
        }
    }
}
