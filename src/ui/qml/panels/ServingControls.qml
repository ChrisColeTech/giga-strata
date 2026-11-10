import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's controls row (04 sections 5 and 7.2): the model picker, Start, Stop,
// Reload, the server state in words (with its failure reason, or the load progress bar while
// loading), and the Parameters toggle at the right end.
RowLayout {
    id: root

    property string modelPath
    property string modelName
    property bool isParametersOpen: false
    readonly property int serverState: LaunchController.state
    readonly property bool isRunning: serverState === LaunchController.Starting || serverState === LaunchController.Loading
                                      || serverState === LaunchController.Serving

    signal modelPicked(string modelPath, string modelName)
    signal parametersToggled

    spacing: Theme.space2

    ModelPicker {
        Layout.preferredWidth: Theme.modelPickerWidth
        Layout.minimumWidth: Theme.modelPickerMinWidth
        modelPath: root.modelPath
        modelName: root.modelName
        onPicked: (modelPath, modelName) => root.modelPicked(modelPath, modelName)
    }

    PrimaryButton {
        text: qsTr("Start")
        iconName: "play"
        enabled: root.modelPath !== "" && !root.isRunning && root.serverState !== LaunchController.Stopping
        toolTipText: root.modelPath === "" ? qsTr("Choose a model first.") : qsTr("Serve %1").arg(root.modelName)
        onClicked: LaunchController.serve(root.modelPath)
    }

    SecondaryButton {
        text: qsTr("Stop")
        iconName: "circle-stop"
        enabled: root.isRunning
        toolTipText: root.isRunning ? qsTr("Stop the server") : qsTr("The server is not running.")
        onClicked: LaunchController.stop()
    }

    SecondaryButton {
        text: qsTr("Reload")
        iconName: "refresh-cw"
        enabled: root.isRunning
        toolTipText: root.isRunning ? qsTr("Restart the server with the model's saved parameters")
                                    : qsTr("The server is not running.")
        onClicked: LaunchController.reload()
    }

    ServerStateText {
        Layout.leftMargin: Theme.space2
    }

    UsageBar {
        fraction: LaunchController.loadProgress < 0 ? -1 : LaunchController.loadProgress / 100
        visible: root.serverState === LaunchController.Loading
        Layout.preferredWidth: Theme.statusMeterWidth
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        text: LaunchController.failureReason
        visible: root.serverState === LaunchController.Failed && text !== ""
        font.family: Theme.uiFamily
        font.pointSize: Theme.bodySize
        color: Theme.text2
        elide: Text.ElideRight
        // Preferred 0: the reason takes the room that is left and elides, never the buttons' room.
        Layout.preferredWidth: 0
        Layout.fillWidth: true
    }

    Item {
        visible: root.serverState !== LaunchController.Failed || LaunchController.failureReason === ""
        Layout.fillWidth: true
    }

    GhostButton {
        text: qsTr("Parameters")
        iconName: "panel-right"
        textColor: root.isParametersOpen ? Theme.accent : Theme.text2
        hoverTextColor: root.isParametersOpen ? Theme.accent : Theme.text1
        toolTipText: root.isParametersOpen ? qsTr("Close the parameters panel") : qsTr("Open the parameters panel")
        Accessible.role: Accessible.CheckBox
        Accessible.checked: root.isParametersOpen
        onClicked: root.parametersToggled()
    }
}
