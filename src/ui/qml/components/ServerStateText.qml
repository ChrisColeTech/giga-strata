import QtQuick
import GigaStrata.App

// The server state in words (LaunchController.stateText) in its semantic color: serving success,
// failed danger, starting and loading info, stopping warn, stopped text3. The status bar and the
// Serving screen both show it, so they always agree.
Text {
    readonly property int serverState: LaunchController.state

    text: LaunchController.stateText
    font.family: Theme.uiFamily
    font.pointSize: Theme.bodySize
    font.weight: Font.DemiBold
    elide: Text.ElideRight
    color: serverState === LaunchController.Serving ? Theme.success
         : serverState === LaunchController.Failed ? Theme.danger
         : serverState === LaunchController.Stopping ? Theme.warn
         : serverState === LaunchController.Stopped ? Theme.text3
         : Theme.info
}
