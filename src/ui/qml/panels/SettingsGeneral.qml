import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Settings > General (04 sections 3 and 7.3), as a property grid: APPEARANCE (reduce motion:
// follow Windows or on) and WINDOW (keep serving in the tray when the window is closed; minimize
// to tray).
Workspace {
    PropertySection {
        title: qsTr("Appearance")
        Layout.topMargin: Theme.space1

        PropertyRow {
            label: qsTr("Reduce motion")
            labelWidth: Theme.propertyLabelWideWidth
            toolTipText: qsTr("Off follows the Windows animation setting; on turns every animation off.")

            Toggle {
                accessibleName: qsTr("Reduce motion")
                isOn: SettingsStore.reduceMotion === "on"
                onSwitched: isOn => SettingsStore.reduceMotion = isOn ? "on" : "system"
            }
        }
    }

    PropertySection {
        title: qsTr("Window")
        Layout.topMargin: Theme.space2

        PropertyRow {
            label: qsTr("Keep serving in the tray")
            labelWidth: Theme.propertyLabelWideWidth
            toolTipText: qsTr("Closing the window while a model is serving hides it to the tray and keeps the server running.")

            Toggle {
                accessibleName: qsTr("Keep serving in the tray")
                isOn: SettingsStore.keepServingInTray
                onSwitched: isOn => SettingsStore.keepServingInTray = isOn
            }
        }

        PropertyRow {
            label: qsTr("Minimize to tray")
            labelWidth: Theme.propertyLabelWideWidth
            toolTipText: qsTr("Minimizing the window hides it to the tray.")

            Toggle {
                accessibleName: qsTr("Minimize to tray")
                isOn: SettingsStore.minimizeToTray
                onSwitched: isOn => SettingsStore.minimizeToTray = isOn
            }
        }
    }
}
