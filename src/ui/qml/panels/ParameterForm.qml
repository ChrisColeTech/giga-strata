import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// A model's parameters as one property grid (04 sections 4 and 7.3): the live memory bars first,
// then MAIN, EXPERT USAGE and ADVANCED scrolling above a footer pinned at the bottom that says
// whether the edits are saved and holds the one commit (Save in the Model config window, Save &
// Reload in the Parameters panel).
ColumnLayout {
    id: root

    property string commitText
    property string commitToolTip

    signal commitRequested

    spacing: 0

    Workspace {
        Layout.fillWidth: true
        Layout.fillHeight: true

        MemorySplit {
            Layout.topMargin: Theme.space1
        }

        ParametersMain {
            Layout.topMargin: Theme.space2
        }

        ParametersExperts {
            Layout.topMargin: Theme.space2
        }

        ParametersAdvanced {
            Layout.topMargin: Theme.space2
        }
    }

    HairLine {}

    RowLayout {
        spacing: Theme.space2
        Layout.fillWidth: true
        Layout.margins: Theme.space2
        Layout.leftMargin: Theme.workspacePadding

        Text {
            text: PlannerRunner.modelPath === "" ? "" : PlannerRunner.isDirty ? qsTr("Unsaved changes") : qsTr("Saved")
            font.family: Theme.uiFamily
            font.pointSize: Theme.captionSize
            color: PlannerRunner.isDirty ? Theme.warn : Theme.text3
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        PrimaryButton {
            text: root.commitText
            toolTipText: root.commitToolTip
            enabled: PlannerRunner.modelPath !== ""
            onClicked: root.commitRequested()
        }
    }
}
