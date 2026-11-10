import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Settings > Server (04 sections 3, 6 and 7.3), as a property grid: off runs the bundled
// llama-server; on picks another llama-server file. Turning it off clears the override.
Workspace {
    id: root

    property bool isOverrideChosen: false
    readonly property string overridePath: SettingsStore.serverOverride
    readonly property bool isOverrideOn: isOverrideChosen || overridePath !== ""

    PropertySection {
        title: qsTr("llama-server")
        Layout.topMargin: Theme.space1

        PropertyRow {
            label: qsTr("Use a different llama-server")
            labelWidth: Theme.propertyLabelWideWidth
            toolTipText: qsTr("Off runs the llama-server bundled with the app, the only build with every flag it uses.")
            status: root.isOverrideOn ? "" : qsTr("bundled")

            Toggle {
                accessibleName: qsTr("Use a different llama-server")
                isOn: root.isOverrideOn
                onSwitched: isOn => {
                    root.isOverrideChosen = isOn
                    if (!isOn) {
                        SettingsStore.serverOverride = ""
                    }
                }
            }
        }

        PropertyRow {
            label: qsTr("llama-server file")
            labelWidth: Theme.propertyLabelWideWidth
            visible: root.isOverrideOn
            isControlWide: true

            PathField {
                placeholderText: "llama-server.exe"
                dialogTitle: qsTr("Locate a llama-server executable")
                nameFilters: [qsTr("Executable files (*.exe)"), qsTr("All files (*)")]
                fieldHeight: Theme.propertyControlHeight
                text: root.overridePath
                startFolder: SettingsStore.folderUrl(root.overridePath)
                Layout.fillWidth: true
                onCommitted: value => SettingsStore.serverOverride = value.trim()
                onPicked: fileUrl => SettingsStore.serverOverride = SettingsStore.nativePath(fileUrl)
            }
        }
    }
}
