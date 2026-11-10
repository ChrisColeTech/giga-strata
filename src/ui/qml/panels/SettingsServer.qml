import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Settings > Server (04 sections 3 and 6): off runs the bundled llama-server; on picks another
// llama-server file. Turning it off clears the override.
Workspace {
    id: root

    property real labelWidth
    property bool isOverrideChosen: false
    readonly property string overridePath: SettingsStore.serverOverride
    readonly property bool isOverrideOn: isOverrideChosen || overridePath !== ""
    readonly property string overrideLabel: qsTr("Use a different llama-server")
    readonly property string fileLabel: qsTr("llama-server file")
    readonly property var labels: [overrideLabel, fileLabel]

    FormGroup {
        title: qsTr("llama-server")
        iconName: "server-cog"
        isLast: true
        Layout.fillWidth: true

        FormRow {
            label: root.overrideLabel
            labelWidth: root.labelWidth
            toolTipText: qsTr("Off runs the llama-server bundled with the app, the only build with every flag it uses.")
            Layout.fillWidth: true

            Toggle {
                accessibleName: root.overrideLabel
                isOn: root.isOverrideOn
                onSwitched: isOn => {
                    root.isOverrideChosen = isOn
                    if (!isOn) {
                        SettingsStore.serverOverride = ""
                    }
                }
            }
        }

        FormRow {
            label: root.fileLabel
            labelWidth: root.labelWidth
            visible: root.isOverrideOn
            Layout.fillWidth: true

            PathField {
                placeholderText: "llama-server.exe"
                dialogTitle: qsTr("Locate a llama-server executable")
                nameFilters: [qsTr("Executable files (*.exe)"), qsTr("All files (*)")]
                text: root.overridePath
                startFolder: SettingsStore.folderUrl(root.overridePath)
                Layout.fillWidth: true
                onCommitted: value => SettingsStore.serverOverride = value.trim()
                onPicked: fileUrl => SettingsStore.serverOverride = SettingsStore.nativePath(fileUrl)
            }
        }
    }
}
