import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Settings window (04 section 3): a work surface (the kit's workSpec: modal, centred, Escape
// unless typing) whose body is the kit's windows/SettingsFrame.tsx, the rail on the left and the
// selected section's property grid beside it (04 section 7.3), over the same footer as a model's
// parameters: Saved or Unsaved changes, Discard and Save. Edits preview at once but reach the
// settings file only on Save; closing the window discards them. Every section uses the same wide
// label column, so it stays put as the rail moves from section to section.
ChildWindow {
    id: root

    readonly property int modelsSection: 1

    function showSection(sectionIndex: int, openerItem: Item) {
        rail.currentIndex = sectionIndex
        root.show(openerItem)
    }

    title: qsTr("Settings")

    onClosed: SettingsStore.discardSettings()

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                SettingsRail {
                    id: rail
                    isWide: root.viewport.width >= Theme.settingsRailWideAt
                    sections: [
                        { label: qsTr("General"), icon: "settings" },
                        { label: qsTr("Models"), icon: "layers" },
                        { label: qsTr("Server"), icon: "server" }
                    ]
                    Layout.fillHeight: true
                }

                StackLayout {
                    currentIndex: rail.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    SettingsGeneral {}
                    SettingsModels {}
                    SettingsServer {}
                }
            }

            HairLine {}

            RowLayout {
                spacing: Theme.space2
                Layout.fillWidth: true
                Layout.margins: Theme.space2
                Layout.leftMargin: Theme.workspacePadding

                Text {
                    text: SettingsStore.hasUnsavedSettings ? qsTr("Unsaved changes") : qsTr("Saved")
                    font.family: Theme.uiFamily
                    font.pointSize: Theme.captionSize
                    color: SettingsStore.hasUnsavedSettings ? Theme.warn : Theme.text3
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                SecondaryButton {
                    text: qsTr("Discard")
                    toolTipText: qsTr("Puts every setting back to what the settings file holds.")
                    enabled: SettingsStore.hasUnsavedSettings
                    onClicked: SettingsStore.discardSettings()
                }

                PrimaryButton {
                    text: qsTr("Save")
                    toolTipText: qsTr("Saves these settings.")
                    enabled: SettingsStore.hasUnsavedSettings
                    onClicked: SettingsStore.saveSettings()
                }
            }
        }
    }
}
