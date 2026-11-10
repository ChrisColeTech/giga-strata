import QtQuick
import QtQuick.Layouts

// The Settings window (04 section 3): a work surface (the kit's workSpec: modal, centred, Escape
// unless typing) whose body is the kit's windows/SettingsFrame.tsx, the rail on the left and the
// selected section's property grid beside it (04 section 7.3). Preferences apply when changed, so
// there is no Save. Every section uses the same wide label column, so it stays put as the rail
// moves from section to section.
ChildWindow {
    id: root

    readonly property int modelsSection: 1

    function showSection(sectionIndex: int, openerItem: Item) {
        rail.currentIndex = sectionIndex
        root.show(openerItem)
    }

    title: qsTr("Settings")

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        RowLayout {
            anchors.fill: parent
            spacing: 0

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
    }
}
