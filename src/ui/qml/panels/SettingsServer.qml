import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Settings > Server (04 sections 3, 6 and 7.3), as a property grid: off runs the bundled
// llama-server; on picks another llama-server file. Turning it off clears the override. GPU
// headroom is the VRAM every model leaves free per GPU.
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

    PropertySection {
        title: qsTr("GPU headroom")
        Layout.topMargin: Theme.space2

        PropertyRow {
            label: qsTr("Headroom (GB)")
            labelWidth: Theme.propertyLabelWideWidth
            toolTipText: qsTr("VRAM every model leaves free on each GPU for other programs, such as the display or another app. A model's own headroom replaces it.")
            disabledReason: MonitorController.gpus.length > 0 ? "" : MonitorController.gpuError

            GpuHeadroomFields {
                text: SettingsStore.gpuHeadroom
                fallbackText: ""
                gpuIndices: MonitorController.gpus.map(gpu => gpu.index)
                onEdited: text => SettingsStore.gpuHeadroom = text
            }
        }
    }
}
