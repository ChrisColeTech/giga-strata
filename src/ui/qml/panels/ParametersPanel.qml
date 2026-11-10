import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's Parameters panel (04 section 5): the chosen model's groups (Main, Expert
// usage, Advanced) and one Save & Reload, which saves to the model's config file and restarts
// the server with it. It slides in from the right edge, resizes by its left edge and closes by
// the toggle or its X.
Rectangle {
    id: root

    signal closeRequested
    signal resized(real delta)

    color: Theme.surface1
    clip: true

    EdgeSizer {
        isVertical: true
        anchors.left: parent.left
        height: parent.height
        z: 1
        onDragged: delta => root.resized(-delta)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.childBorder
        spacing: 0

        RowLayout {
            spacing: Theme.space2
            Layout.fillWidth: true
            Layout.leftMargin: Theme.space4
            Layout.rightMargin: Theme.space1
            Layout.preferredHeight: Theme.controlHeight + Theme.space2

            Text {
                text: qsTr("Parameters")
                font.family: Theme.uiFamily
                font.pointSize: Theme.bodySize
                font.weight: Font.DemiBold
                color: Theme.text1
                Layout.fillWidth: true
            }

            IconButton {
                iconName: "x"
                toolTipText: qsTr("Close the parameters panel")
                onClicked: root.closeRequested()
            }
        }

        Rectangle {
            color: Theme.border
            implicitHeight: 1
            Layout.fillWidth: true
        }

        Workspace {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ParameterForm {
                commitText: qsTr("Save & Reload")
                commitToolTip: qsTr("Saves these parameters to the model's config file and restarts the server with them.")
                Layout.fillWidth: true
                onCommitRequested: {
                    if (PlannerRunner.save()) {
                        LaunchController.serve(PlannerRunner.modelPath)
                    }
                }
            }
        }
    }
}
