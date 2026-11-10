import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's Parameters panel (04 sections 5 and 7.3): a "PARAMETERS — <model>" header
// with its X, the model's property grid, and the footer's Save & Reload, which saves to the
// model's config file and restarts the server with it. It slides in from the right edge, resizes
// by its left edge and closes by the toggle or its X.
Rectangle {
    id: root

    property string modelName

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

    Rectangle {
        anchors.left: parent.left
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.childBorder
        spacing: 0

        RowLayout {
            spacing: Theme.space2
            Layout.fillWidth: true
            Layout.leftMargin: Theme.workspacePadding
            Layout.rightMargin: Theme.space1
            Layout.preferredHeight: Theme.propertyRowHeight + Theme.space1

            Overline {
                text: qsTr("Parameters")
            }

            Text {
                text: "— " + root.modelName
                visible: root.modelName !== ""
                font.family: Theme.uiFamily
                font.pointSize: Theme.captionSize
                color: Theme.text2
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            Item {
                visible: root.modelName === ""
                Layout.fillWidth: true
            }

            IconButton {
                iconName: "x"
                implicitWidth: Theme.propertyRowHeight
                implicitHeight: Theme.propertyRowHeight
                toolTipText: qsTr("Close the parameters panel")
                onClicked: root.closeRequested()
            }
        }

        HairLine {}

        ParameterForm {
            commitText: qsTr("Save & Reload")
            commitToolTip: qsTr("Saves these parameters to the model's config file and restarts the server with them.")
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCommitRequested: {
                if (PlannerRunner.save()) {
                    LaunchController.serve(PlannerRunner.modelPath)
                }
            }
        }
    }
}
