import QtQuick
import QtQuick.Layouts

// The Settings window (04 section 3): a work surface (the kit's workSpec: modal, centred, Escape
// unless typing) whose body is the kit's windows/SettingsFrame.tsx, the rail on the left and the
// selected section's form beside it on the background color. Preferences apply when changed, so
// there is no Save. One label column for every label the window can show (lib/label-column.ts),
// so it stays put as the rail moves from section to section.
ChildWindow {
    id: root

    readonly property int modelsSection: 1
    readonly property var sectionPages: [general, models, server, about]
    readonly property real labelWidth: labelColumn.width

    function showSection(sectionIndex: int, openerItem: Item) {
        rail.currentIndex = sectionIndex
        root.show(openerItem)
    }

    title: qsTr("Settings")

    LabelColumn {
        id: labelColumn
        labels: root.sectionPages.reduce((all, page) => all.concat(page.labels), [])
    }

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
                    { label: qsTr("Server"), icon: "server" },
                    { label: qsTr("About"), icon: "info" }
                ]
                Layout.fillHeight: true
            }

            StackLayout {
                currentIndex: rail.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true

                SettingsGeneral {
                    id: general
                    labelWidth: root.labelWidth
                }
                SettingsModels {
                    id: models
                    labelWidth: root.labelWidth
                }
                SettingsServer {
                    id: server
                    labelWidth: root.labelWidth
                }
                SettingsAbout {
                    id: about
                    labelWidth: root.labelWidth
                }
            }
        }
    }
}
