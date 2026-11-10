import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Settings > General (04 section 3): Appearance (reduce motion: follow Windows or on) and Window
// (keep serving in the tray when the window is closed; minimize to tray).
Workspace {
    id: root

    property real labelWidth
    readonly property string reduceMotionLabel: qsTr("Reduce motion")
    readonly property string keepServingLabel: qsTr("Keep serving in the tray")
    readonly property string minimizeLabel: qsTr("Minimize to tray")
    readonly property var labels: [reduceMotionLabel, keepServingLabel, minimizeLabel]

    FormGroup {
        title: qsTr("Appearance")
        iconName: "palette"
        Layout.fillWidth: true

        FormRow {
            label: root.reduceMotionLabel
            labelWidth: root.labelWidth
            toolTipText: qsTr("Off follows the Windows animation setting; on turns every animation off.")
            Layout.fillWidth: true

            Toggle {
                accessibleName: root.reduceMotionLabel
                isOn: SettingsStore.reduceMotion === "on"
                onSwitched: isOn => SettingsStore.reduceMotion = isOn ? "on" : "system"
            }
        }
    }

    FormGroup {
        title: qsTr("Window")
        iconName: "app-window"
        isLast: true
        Layout.fillWidth: true

        FormRow {
            label: root.keepServingLabel
            labelWidth: root.labelWidth
            toolTipText: qsTr("Closing the window while a model is serving hides it to the tray and keeps the server running.")
            Layout.fillWidth: true

            Toggle {
                accessibleName: root.keepServingLabel
                isOn: SettingsStore.keepServingInTray
                onSwitched: isOn => SettingsStore.keepServingInTray = isOn
            }
        }

        FormRow {
            label: root.minimizeLabel
            labelWidth: root.labelWidth
            toolTipText: qsTr("Minimizing the window hides it to the tray.")
            Layout.fillWidth: true

            Toggle {
                accessibleName: root.minimizeLabel
                isOn: SettingsStore.minimizeToTray
                onSwitched: isOn => SettingsStore.minimizeToTray = isOn
            }
        }
    }
}
